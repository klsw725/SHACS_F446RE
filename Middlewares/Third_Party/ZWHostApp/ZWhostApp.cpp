/****************************************************************************
*
* Z-Wave, the wireless language.
* 
* Copyright (c) 2012
* Sigma Designs, Inc.
* All Rights Reserved
*
* This source file is subject to the terms and conditions of the
* Sigma Designs Software License Agreement which restricts the manner
* in which it may be used.
*
*---------------------------------------------------------------------------
*
* Description:      Z-Wave Network Health Toolbox using Serial API.
*
* Last Changed By:  $Author$: 
* Revision:         $Rev$: 
* Last Changed:     $Date$: 
* History
-  2017.11.06 Modified by JY Koh for SHACS project
****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <UsbSerial.h>
#include <ZwSerialAPI.h>
#include <ZWhostApp.h>

#include "networkManagement.h"
#include "timing.h"
#include "ZW_SerialAPI.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "BtSerial.h"
#include "UsbSerial.h"
#include "WifiSerial.h"

#define	sprintf_s	snprintf
#define	printf_s	printf
#define	mainlog_wr	printf
#define	debuglog_wr	printf
#define	MAXLONG		40

#define		ZW_UART_PORT	1

extern	UART_HandleTypeDef huart2;
extern	osTimerId ZWhostAppTimerHandle;

int UARTgetsNonblocking(char *pcBuf, uint32_t ui32Len);

/* We wait max 8 sec for a Version Report */
#define GETVERSIONREPORTTIMEOUT	8

CSerialAPI api;

sNetworkManagement sNetworkManagementUT;

CTime* pTimer;
int timerH = 0;

CTime *pGetVersionReportTimer;
int timerGetVersionReportH = 0;

unsigned int bNodeID = 1;

S_SERIALAPI_CAPABILITIES sController_SerialAPI_Capabilities;
BYTE bSerialAPIVer = 0;
BYTE bDeviceCapabilities = 0;
BYTE bNodeExistMaskLen = 0;
BYTE bNodeExistMask[ZW_MAX_NODEMASK_LENGTH];
BYTE bControllerCapabilities;
BYTE MySystemSUCID;

BYTE nodeType[ZW_MAX_NODES];
BYTE nodeList[ZW_MAX_NODES];
BYTE nodeListSize;
BYTE abListeningNodeList[232];
BYTE bListeningNodeListSize;
bool afIsNodeAFLiRS[232];
BYTE abPingFailed[232];
BYTE abPingFailedSize;
BYTE bPingNodeIndex;
BYTE bNetworkRepeaterCount;
BYTE lastLearnedNodeID;
BYTE lastLearnedNodeType;
BYTE lastRemovedNodeID;
BYTE lastRemovedNodeType;
BOOL learnMode_status = false;
BYTE MyNodeId;
BYTE MySystemHomeId[4];
BYTE bLWRLocked = false;

BOOL bAddRemoveNodeFromNetwork = false;

BOOL testStarted = false;
BYTE testMode = 0;

static BYTE pBasicBuf[64];
char bCharBuf[1024] = "";

static time_t startTime;
static time_t endTime;

bool printIncomming = true;

bool screenLog = false;


/* logging parameters */
char main_logfilenameStr[20];
char debug_logfilenameStr[20];



void GetVersionReportTimeout(int H);

void RequestNextNodeInformation(int H);
void DoRequestForNodeInformation(BYTE bNextIndex);
bool NetworkHealthStart();
char* WriteNodeTypeString(BYTE nodeType);

BYTE	UpperRsp[32];
BYTE 	Control_from = 0;

void report_Upper(BYTE report, BYTE info)
{
  if (Control_from == 1)
  {
    UpperRsp[0] = 2;
    UpperRsp[1] = report;
    UpperRsp[2] = info;
    put_BTresponse(UpperRsp, 3);
  }
  else if (Control_from == 2)
  {
    UpperRsp[0] = 0xAB;
    UpperRsp[1] = 0xAD;
    UpperRsp[2] = report;
    UpperRsp[3] = info;
    
    put_USBresponse(UpperRsp, 4);
  }
  printf ("Report Info to Upper(%d) [%02X][%02X]\r\n", Control_from, report, info);
}

void response_Upper(BYTE response, BYTE result, BYTE ID)
{
  BYTE	length;
  
  UpperRsp[0] = 0xAB;
  UpperRsp[1] = 0xAD;
  UpperRsp[2] = response;
  UpperRsp[3] = result;
  UpperRsp[4] = ID;
  
  if	(response == 6 || response == 7)
    length = 4;
  else
    length = 5;
  
  if (Control_from == 1)
  {
    UpperRsp[0] = length -1;
    UpperRsp[1] = response;
    UpperRsp[2] = result;
    UpperRsp[3] = ID;
    put_BTresponse(UpperRsp, (length - 1));
  }
  else if (Control_from == 2)
  {
    UpperRsp[0] = 0xAB;
    UpperRsp[1] = 0xAD;
    UpperRsp[2] = response;
    UpperRsp[3] = result;
    UpperRsp[4] = ID;
    put_USBresponse(UpperRsp, length);
  }
  if (length == 4)
    printf ("Response Info to Upper(%d) [%02X][%02X]\r\n", Control_from, response, result);
  else
    printf ("Response Info to Upper(%d) [%02X][%02X][%02X]\r\n", Control_from, response, result, ID);
}

/*==================================  mainlog_wr =============================
** Function description
**      mlog.Write wrapper for writing to mainlog
**
** Side effects:
**		
**--------------------------------------------------------------------------*/


/*==================================  debuglog_wr ============================
** Function description
**      mlog.Write wrapper for writing to debuglog
**
** Side effects:
**		
**--------------------------------------------------------------------------*/


/*==============================   GetValidLibTypeStr ========================
** Function description
**      Returns pointer to string describing the library type if libType is a
**	    valid Z-Wave (ZW_LIB_...) library type
**		Returns pointer to empty string ("") if libType is NOT a 
**		valid Z-Wave (ZW_LIB_...) library type
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
char*
GetValidLibTypeStr(
                   BYTE libType)
{
  char *pRetStr;
  switch (libType)
  {
  case ZW_LIB_CONTROLLER_STATIC:
    {
      pRetStr = "Controller Static";
    }
    break;
    
  case ZW_LIB_CONTROLLER_BRIDGE:
    {
      pRetStr = "Controller Bridge";
    }
    break;
    
  case ZW_LIB_CONTROLLER:
    {
      pRetStr = "Controller Portable";
    }
    break;
    
  case ZW_LIB_SLAVE_ENHANCED:
    {
      pRetStr = "Slave Enhanced";
    }
    break;  
    
  case ZW_LIB_SLAVE_ROUTING:
    {
      pRetStr = "Slave Routing";
    }
    break;  
    
  case ZW_LIB_SLAVE:
    {
      pRetStr = "Slave";
    }
    break;  
    
  case ZW_LIB_INSTALLER:
    {
      pRetStr = "Controller Installer";
    }
    break;
    
  default:
    {
      pRetStr = "";
    }
    break;  
  }
  return pRetStr;
}


/*========================   InitializeSerialAPICapabilities =================
** Function description
**      Initialize sController_SerialAPI_Capabilities with the attached
**	module SerialAPI capabilities using api.SerialAPI_Get_Capabilities()
**		a SUC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
InitializeSerialAPICapabilities(void)
{
  BYTE abController_SerialAPI_Capabilities[64];
  
  memset(&abController_SerialAPI_Capabilities, 0, sizeof(abController_SerialAPI_Capabilities));
  memset((BYTE*)&sController_SerialAPI_Capabilities, 0, sizeof(sController_SerialAPI_Capabilities));
  api.SerialAPI_Get_Capabilities((BYTE*)&abController_SerialAPI_Capabilities);
  /* If Application Version and Revision both are ZERO then we assume we did not get any useful information */
  if ((0 != abController_SerialAPI_Capabilities[0]) || (0 != abController_SerialAPI_Capabilities[1]))
  {
    memcpy((BYTE*)&sController_SerialAPI_Capabilities, &abController_SerialAPI_Capabilities, sizeof(sController_SerialAPI_Capabilities));
  }
}


/*============================   IdleLearnNodeState_Compl ====================
** Function description
**      Callback which is called when a node is added or removed by
**		a SUC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
IdleLearnNodeState_Compl(
                         BYTE bStatus, /*IN Status of learn mode*/
                         BYTE bSource, /*IN Source node*/
                         BYTE *pCmd,   /*IN Data*/
                         BYTE bLen)    /*Length of data*/
{
  static BYTE lastLearnedNodeType;
  if (bLen)
  {
    NODEINFO nodeInfo;
    api.ZW_GetNodeProtocolInfo(bSource, &nodeInfo);
    if (nodeInfo.nodeType.basic < BASIC_TYPE_SLAVE)
    {
      lastLearnedNodeType = nodeInfo.nodeType.basic;  /*For now we store the learned basic node type*/
    }
    else
    {
      lastLearnedNodeType = nodeInfo.nodeType.generic; /* Store learned generic node type */
    }
    if (0 < bSource)
    {
      memcpy(&sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeInfo.capability, &nodeInfo.capability, sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeInfo));
    }
  }
  
  if (bStatus == UPDATE_STATE_ROUTING_PENDING)
  {
    mainlog_wr("waiting ...\r\n");
  }
  else if (bStatus == UPDATE_STATE_ADD_DONE)
  {
    mainlog_wr("Node %u included ... type %u\r\n", bSource, lastLearnedNodeType);
  }
  else if (bStatus == UPDATE_STATE_DELETE_DONE)
  {
    mainlog_wr("Node %u removed  ... type %u\r\n", bSource, lastLearnedNodeType);
  }
  else if (bStatus == UPDATE_STATE_NODE_INFO_RECEIVED)
  {
    mainlog_wr("Node %u sends nodeinformation\r\n", bSource);
    if ((0 < bSource) && (bLen >= sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType)))
    {
      memcpy(&sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType.basic, pCmd, sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType));
      memcpy(sNetworkManagementUT.nodeDescriptor[bSource - 1].cmdClasses, (BYTE *)(pCmd + sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType)), bLen - sizeof(sNetworkManagementUT.nodeDescriptor[bSource - 1].nodeType));
      if (0 != timerH)
      {
        pTimer->Kill(timerH);
        timerH = 0;
        RequestNextNodeInformation(0);
      }
    }
  }
  else
  {
    mainlog_wr("Node %u sends %02x\r\n", bSource, *pCmd);
  }
}


void
ApplicationCommandHandler(
                          BYTE rxStatus, 
                          BYTE sourceNode, 
                          BYTE *pCmd, 
                          BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER *pdata = (ZW_APPLICATION_TX_BUFFER *) pCmd;
  char *frameType;
  
  switch (rxStatus & RECEIVE_STATUS_TYPE_MASK)
  {
  case RECEIVE_STATUS_TYPE_SINGLE:
    frameType = "singlecast";
    break;
    
  case RECEIVE_STATUS_TYPE_BROAD:
    frameType = "broadcast";
    break;
    
  case RECEIVE_STATUS_TYPE_MULTI:
    frameType = "multicast";
    break;
    
  case RECEIVE_STATUS_TYPE_EXPLORE:
    frameType = "explorer";
    break;
    
  default:
    frameType = "Unknown frametype";
    break;
  }
  switch(*(pCmd+CMDBUF_CMDCLASS_OFFSET))
  {
  case COMMAND_CLASS_VERSION:
    {
      switch (*(pCmd+CMDBUF_CMD_OFFSET))
      {
      case VERSION_REPORT:
        {
          char *pLibTypeStr;
          pLibTypeStr = GetValidLibTypeStr(*(pCmd+CMDBUF_PARM1_OFFSET));
          mainlog_wr("Node %u says VERSION REPORT (%s)\r\n", sourceNode, frameType);
          mainlog_wr("Z-Wave %s protocol v%02u.%02u, App. v%02u.%02u\r\n", pLibTypeStr,
                     *(pCmd+CMDBUF_PARM2_OFFSET),
                     *(pCmd+CMDBUF_PARM3_OFFSET),
                     *(pCmd+CMDBUF_PARM4_OFFSET),
                     *(pCmd+CMDBUF_PARM5_OFFSET));
          /* If timer is running, kill it! */
          if (0 != timerGetVersionReportH)
          {
            pGetVersionReportTimer->Kill(timerGetVersionReportH);
          }
          timerGetVersionReportH = 0;
          /* We assume the answer we just received was the one we asked for... */
          GetVersionReportTimeout(0);
        }
        break;
        
      default:
        mainlog_wr("Node %u says VERSION command %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
        break;
      }
    }
    break;
    
    //        case COMMAND_CLASS_POWERLEVEL:
    //            CmdHandlerPowerLevel(rxStatus, sourceNode, pCmd, cmdLength);
    //            break;
    
  case COMMAND_CLASS_SWITCH_MULTILEVEL:
    if (printIncomming)
    {
      if(*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_REPORT)
      {
        mainlog_wr("Node %u says SWITCH MULTILEVEL REPORT %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
      }
      else if (*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_SET)
      {
        mainlog_wr("Node %u says SWITCH MULTILEVEL SET %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
      }
      else if (*(pCmd+CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_GET)
      {
        mainlog_wr("Node %u says SWITCH MULTILEVEL GET (%s)\r\n", sourceNode, frameType);
      }
    }
    break;
    
  case COMMAND_CLASS_BASIC:
    switch (*(pCmd+CMDBUF_CMD_OFFSET))
    {
    case BASIC_REPORT:
      {
        if (printIncomming)
        {
          mainlog_wr("Node %u says BASIC REPORT %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
        }
      }
      break;
      
    case BASIC_SET:
      {
        if (printIncomming)
        {
          mainlog_wr("Node %u says BASIC SET %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
        }
      }
      break;
      
    case BASIC_GET:
      {
        if (printIncomming)
        {
          mainlog_wr("Node %u says BASIC GET (%s)\r\n", sourceNode, frameType);
        }
      }
      break;
      
    default:
      {
        if (printIncomming)
        {
          mainlog_wr("Node %u says none supported BASIC command %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
        }
      }
      break;
    }
    break;
    
    
  case COMMAND_CLASS_SENSOR_BINARY:
    if (*(pCmd + CMDBUF_CMD_OFFSET) == SENSOR_BINARY_REPORT)
    {
    }
    break;
    
  case COMMAND_CLASS_CONTROLLER_REPLICATION:
    api.ZW_ReplicationCommandComplete();
    break;
    
  case COMMAND_CLASS_METER:
    if (*(pCmd + CMDBUF_CMD_OFFSET) == METER_REPORT)
    {
      if (printIncomming)
      {
        mainlog_wr("Node %u says METER REPORT (%s)\r\n", sourceNode, frameType);
      }
    }
    break;
    
  default:
    if (printIncomming)
    {
      mainlog_wr("Node %u says commandclass %u, command %u (%s)\r\n", sourceNode, *(pCmd+CMDBUF_CMDCLASS_OFFSET), *(pCmd+CMDBUF_CMD_OFFSET), frameType);
    }
    break;
  }
}


void
ApplicationCommandHandler_Bridge(
                                 BYTE rxStatus,
                                 BYTE destNode,
                                 BYTE sourceNode, 
                                 BYTE *multi,
                                 BYTE *pCmd, 
                                 BYTE cmdLength)
{
  ZW_APPLICATION_TX_BUFFER *pdata = (ZW_APPLICATION_TX_BUFFER *) pCmd;
  char *frameType;
  
  switch (rxStatus & RECEIVE_STATUS_TYPE_MASK)
  {
  case RECEIVE_STATUS_TYPE_SINGLE:
    frameType = "virtual singlecast";
    break;
    
  case RECEIVE_STATUS_TYPE_BROAD:
    frameType = "virtual broadcast";
    break;
    
  case RECEIVE_STATUS_TYPE_MULTI:
    frameType = "virtual multicast";
    break;
    
  default:
    frameType = "virtual Unknown frametype";
    break;
  }
  mainlog_wr("Node %u says to %u - %u, %u (%s)\r\n", sourceNode, destNode, *(pCmd+CMDBUF_CMDCLASS_OFFSET), *(pCmd+CMDBUF_PARM1_OFFSET), frameType);
  if ((NULL != multi) && (0 < multi[0]))
  {
    sprintf_s(bCharBuf, sizeof(bCharBuf), " Nodemask: ");
    for (int i = 1; i <= multi[0]; i++)
    {
      sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf)-strlen(bCharBuf),"%0X ", multi[i]);
    }
    mainlog_wr(bCharBuf);
    sprintf_s(bCharBuf, sizeof(bCharBuf), "Destination :");
    int lNodeID = 1;
    int bitmask;
    for (int i = 1; i <= multi[0]; i++)
    {
      bitmask = 0x01;
      for (int j = 1; j <= 8; j++, bitmask <<= 1)
      {
        if (bitmask & multi[i])
        {
          sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf)-strlen(bCharBuf), " %u", lNodeID);
        }
        lNodeID++;
      }
    }
    mainlog_wr(bCharBuf);
  }
  mainlog_wr("* ");
  switch (*(pCmd + CMDBUF_CMDCLASS_OFFSET))
  {
  case COMMAND_CLASS_SWITCH_MULTILEVEL:
    if (*(pCmd + CMDBUF_CMD_OFFSET) == SWITCH_MULTILEVEL_REPORT)
    {
      mainlog_wr("SWITCH MULTILEVEL REPORT(SNode-%d) %02X\r\n", sourceNode, *(pCmd + CMDBUF_PARM1_OFFSET));
    }
    else if (*(pCmd + CMDBUF_CMD_OFFSET)== SWITCH_MULTILEVEL_SET)
    {
      mainlog_wr("SWITCH MULTILEVEL SET(SNode-%d) %02X\r\n", sourceNode, *(pCmd + CMDBUF_PARM1_OFFSET));
    }
    else if (*(pCmd + CMDBUF_CMD_OFFSET) == SWITCH_MULTILEVEL_GET)
    {
      mainlog_wr("SWITCH MULTILEVEL GET(SNode-%d)\r\n", sourceNode);
    }
    else
    {
      mainlog_wr("None supported SWITCH MULTILEVEL command %02X\r\n", *(pCmd + CMDBUF_CMD_OFFSET));
    }
    break;
    
  case COMMAND_CLASS_BASIC:
    switch (*(pCmd + CMDBUF_CMD_OFFSET))
    {
    case BASIC_REPORT:
      {
        mainlog_wr("BASIC REPORT %02X\r\n", *(pCmd + CMDBUF_PARM1_OFFSET));
      }
      break;
      
    case BASIC_SET:
      {
        mainlog_wr("BASIC SET %02X\r\n", *(pCmd + CMDBUF_PARM1_OFFSET));
      }
      break;
      
    case BASIC_GET:
      {
        mainlog_wr("BASIC GET\r\n");
      }
      break;
      
    default:
      {
        mainlog_wr("None supported BASIC command %02X\r\n", *(pCmd + CMDBUF_CMD_OFFSET));
      }
      break;
    }
    break;
    
    
  case COMMAND_CLASS_SENSOR_BINARY:
    if (*(pCmd + CMDBUF_CMD_OFFSET) == SENSOR_BINARY_REPORT)
    {
      mainlog_wr("SENSOR BINARY REPORT %02X\r\n", *(pCmd + CMDBUF_PARM1_OFFSET));
    }
    else
    {
      mainlog_wr("None supported SENSOR BINARY command %02X\r\n", *(pCmd + CMDBUF_CMD_OFFSET));
    }
    break;
    
  case COMMAND_CLASS_CONTROLLER_REPLICATION:
    mainlog_wr("CONTROLLER REPLICATION\r\n");
    api.ZW_ReplicationCommandComplete();
    break;
    
  default:
    mainlog_wr("Commandclass %02X, Command %02X\r\n", *(pCmd + CMDBUF_CMDCLASS_OFFSET), *(pCmd + CMDBUF_CMD_OFFSET));
    break;
  }
}


/*============================   CommErrorNotification =======================
** Function description
**      Called when a communication error occours between Z-Wave module and
**		PC
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
void 
CommErrorNotification(
                      BYTE byReason)
{
  switch (byReason)
  {
  case CSerialAPI::COMM_RETRY_EXCEEDED:
    mainlog_wr("Communication with Z-Wave module was lost(%s) while sending command.\r\nCheck the cable and either retry the operation or restart the application.\r\n", "Error");
    break;
    
  case CSerialAPI::COMM_NO_RESPONSE:
    mainlog_wr("Communication with Z-Wave module timed out(%s) while waiting for response.\r\nCheck the cable and either retry the operation or restart the application.\r\n", "Error");
    break;
    
  default:
    mainlog_wr("Communication with Z-Wave module was lost(%s).\r\nCheck the cable and either retry the operation or restart the application.\r\n", "Error");
    break;
  }
}


/*============================   WriteNodeTypeString ========================
** Function description
**      Returns a CString containing a fitting name to the nodetype supplied
** Side effects:
**		
**--------------------------------------------------------------------------*/
char* 
WriteNodeTypeString(
                    BYTE nodeType)
{
  switch (nodeType)
  {
  case GENERIC_TYPE_GENERIC_CONTROLLER:
    return "Generic Controller";
    
  case GENERIC_TYPE_STATIC_CONTROLLER:
    return "Static Controller";
    
  case GENERIC_TYPE_REPEATER_SLAVE:
    return "Repeater Slave";
    
  case GENERIC_TYPE_SWITCH_BINARY:
    return "Binary Switch";
    
  case GENERIC_TYPE_SWITCH_MULTILEVEL:
    return "Multilevel Switch";
    
  case GENERIC_TYPE_SWITCH_REMOTE:
    return "Remote Switch";
    
  case GENERIC_TYPE_SWITCH_TOGGLE:
    return "Toggle Switch";
    
  case GENERIC_TYPE_SENSOR_BINARY:
    return "Binary Sensor";
    
  case GENERIC_TYPE_SENSOR_MULTILEVEL:
    return "Sensor Multilevel";
    
  case GENERIC_TYPE_SENSOR_ALARM:
    return "Sensor Alarm";
    
  case GENERIC_TYPE_METER:
    return "Meter";
    
  case GENERIC_TYPE_METER_PULSE:
    return "Pulse Meter";
    
  case GENERIC_TYPE_ENTRY_CONTROL:
    return "Entry Control";
    
  case GENERIC_TYPE_AV_CONTROL_POINT:
    return "AV Control Point";
    
  case GENERIC_TYPE_DISPLAY:
    return "Display";
    
  case GENERIC_TYPE_SEMI_INTEROPERABLE:
    return "Semi Interoperable";
    
  case GENERIC_TYPE_NON_INTEROPERABLE:
    return "Non Interoperable";
    
  case GENERIC_TYPE_THERMOSTAT:
    return "Thermostat";
    
  case GENERIC_TYPE_VENTILATION:
    return "Ventilation";
    
  case GENERIC_TYPE_WINDOW_COVERING:
    return "Window Covering";
    
  case GENERIC_TYPE_SECURITY_PANEL:
    return "Security Panel";
    
  case GENERIC_TYPE_WALL_CONTROLLER:
    return "Wall Controller";
    
  case GENERIC_TYPE_APPLIANCE:
    return "Appliance";
    
  case GENERIC_TYPE_SENSOR_NOTIFICATION:
    return "Sensor Notification";
    
  case GENERIC_TYPE_NETWORK_EXTENDER:
    return "Network Extender";
    
  case GENERIC_TYPE_ZIP_NODE:
    return "Zip Node";
    
  case 0:			/* No Nodetype registered */
    return "No device";
    
  default:
    return "Unknown Device type";
  }
}


/*============================ SetLearnNodeStateDelete_Compl ================
** Function description
**      Callback function for when deleting a node
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SetLearnNodeStateDelete_Compl(
                              BYTE bStatus, 
                              BYTE bSource, 
                              BYTE *pCmd, 
                              BYTE bLen)
{
  switch(bStatus)
  {
  case REMOVE_NODE_STATUS_LEARN_READY:
    mainlog_wr("RNN: [REMOVE_NODE_STATUS_LEARN_READY(%02X)] - Waiting... (Nodeinformation length %u)\r\n", bStatus, bLen);
    break;
    
  case REMOVE_NODE_STATUS_NODE_FOUND:
    mainlog_wr("RNN: [REMOVE_NODE_STATUS_NODE_FOUND(%02X)]  - Node found %03u (Nodeinformation length %u)\r\n", bStatus, bSource, bLen);
    break;
    
  case REMOVE_NODE_STATUS_REMOVING_SLAVE:
    {
      NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
      if (bLen == 0)
      {
        lastRemovedNodeType = 0;
      }
      else
      {
        if (node_Type.basic < BASIC_TYPE_SLAVE)
        {
          lastRemovedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
        }
        else
        {
          lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
        }
      }
      lastRemovedNodeID = bSource;
      mainlog_wr("RNN: [REMOVE_NODE_STATUS_REMOVING_SLAVE(%02X)]  - Removing Slave Node %03u (%s)... (Nodeinformation length %u)\r\n", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), bLen);
      if (bSource)
      {
        nodeType[bSource - 1] = 0;
      }
      api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
    }
    break;
    
  case REMOVE_NODE_STATUS_REMOVING_CONTROLLER:
    {
      NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
      if (bLen == 0)
      {
        lastRemovedNodeType = 0;
      }
      else
      {
        if (node_Type.basic < BASIC_TYPE_SLAVE)
        {
          lastRemovedNodeType = node_Type.basic;  /* For now we store the learned basic node type */
        }
        else
        {
          lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
        }
      }
      lastRemovedNodeID = bSource;
      mainlog_wr("RNN: [REMOVE_NODE_STATUS_REMOVING_CONTROLLER(%02X)]  - Removing Controller Node %03u (%s)... (Nodeinformation length %u)\r\n", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), bLen);
      if (bSource)
      {
        nodeType[bSource - 1] = 0;
      }
      api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
    }
    break;
    
  case REMOVE_NODE_STATUS_DONE:
    {
      NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
      if (bLen == 0)
      {
        lastRemovedNodeType = 0;
      }
      else
      {
        if (node_Type.basic < BASIC_TYPE_SLAVE)
        {
          lastRemovedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
        }
        else
        {
          lastRemovedNodeType = node_Type.generic; /* Store learned generic node type*/
        }
      }
      lastRemovedNodeID = bSource;
      mainlog_wr("RNN: [REMOVE_NODE_STATUS_DONE(%02X)]  - Removing Node %03u (%s(%02X))... (Nodeinformation length %u)\r\n", bStatus, bSource, WriteNodeTypeString(lastRemovedNodeType), lastRemovedNodeType, bLen);
      if (bSource)
      {
        nodeType[bSource - 1] = 0;
      }
      /* TO#3932 fix */
      bAddRemoveNodeFromNetwork = false;
      api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
    }
    break;
    
  case REMOVE_NODE_STATUS_FAILED:
    {
      mainlog_wr("RNN: [REMOVE_NODE_STATUS_FAILED(%02X)]  - Remove Node failed... (Nodeinformation length %u)\r\n", bStatus, bLen);
      bAddRemoveNodeFromNetwork = false;
      api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
    }
    break;
    
  default:
    {
      /* If were not pending, we want to turn off learn mode... */
      mainlog_wr("RNN: [%02X]  - Unknown status, stopping... (Nodeinformation length %u)\r\n", bStatus, bLen);
      bAddRemoveNodeFromNetwork = false;
      api.ZW_AddNodeToNetwork(REMOVE_NODE_STOP, NULL);
    }
    break;
  }
}


/*============================ SetLearnNodeState_Compl ======================
** Function description
**      Called when a new node have been added
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SetLearnNodeState_Compl(
                        BYTE bStatus, /*IN Status of learn process*/
                        BYTE bSource, /*IN Node ID of node learned*/
                        BYTE *pCmd,	  /*IN Pointer to node information*/
                        BYTE bLen)	  /*IN Length of node information*/
{
  switch(bStatus)
  {
  case ADD_NODE_STATUS_LEARN_READY:
    
    mainlog_wr("ANN: [ADD_NODE_STATUS_LEARN_READY(%02X)]      - Press node button... (Nodeinformation length %u)\r\n", bStatus, bLen);
    report_Upper(0x14, 0);
    break;
    
  case ADD_NODE_STATUS_NODE_FOUND:
    mainlog_wr("ANN: [ADD_NODE_STATUS_NODE_FOUND(%02X)]       - Waiting ... (Nodeinformation length %u)\r\n", bStatus, bLen);
    break;
    
  case ADD_NODE_STATUS_ADDING_SLAVE:
    {
      NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
      if (bLen == 0)
      {
        lastRemovedNodeType = 0;
      }
      else
      {
        if (node_Type.basic < BASIC_TYPE_SLAVE)
        {
          lastLearnedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
        }
        else
        {
          lastLearnedNodeType = node_Type.generic; /* Store learned generic node type*/
        }
      }
      lastLearnedNodeID = bSource;
      mainlog_wr("ANN: [ADD_NODE_STATUS_ADDING_SLAVE(%02X)]     - Adding Slave unit... Nodeinformation length %u\r\n", bStatus, bLen);
      
    }
    break;
    
  case ADD_NODE_STATUS_ADDING_CONTROLLER:
    {
      NODE_TYPE node_Type = *((NODE_TYPE*)pCmd);
      if (bLen == 0)
      {
        lastRemovedNodeType = 0;
      }
      else
      {
        if (node_Type.basic < BASIC_TYPE_SLAVE)
        {
          lastLearnedNodeType = node_Type.basic;  /*For now we store the learned basic node type*/
        }
        else
        {
          lastLearnedNodeType = node_Type.generic; /* Store learned generic node type*/
        }
      }
      lastLearnedNodeID = bSource;
      mainlog_wr("ANN: [ADD_NODE_STATUS_ADDING_CONTROLLER(%02X)] - Adding Controller unit... Nodeinformation length %u\r\n", bStatus, bLen);
    }
    break;
    
  case ADD_NODE_STATUS_PROTOCOL_DONE:
    api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
    mainlog_wr("ANN: [ADD_NODE_STATUS_PROTOCOL_DONE(%02X)]    - Now stopping, app has nothing... (Nodeinformation length %u)\r\n", bStatus, bLen);
    break;
    
  case ADD_NODE_STATUS_DONE:
    /* Hmm properly not needed... */
    api.ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
    mainlog_wr("ANN: [ADD_NODE_STATUS_DONE(%02X)]             - Node included, stopping... %u - %s(%02X) (Nodeinformation length %u)\r\n", bStatus, lastLearnedNodeID, WriteNodeTypeString(lastLearnedNodeType), lastLearnedNodeType, bLen);
    nodeType[lastLearnedNodeID-1] = lastLearnedNodeType;
    bAddRemoveNodeFromNetwork = false;
    if (lastLearnedNodeType == 0)
      response_Upper(4, 0, 0);
    else
      response_Upper(4, 1, lastLearnedNodeID);
    break;
    
  case ADD_NODE_STATUS_FAILED:
    mainlog_wr("ANN: [ADD_NODE_STATUS_FAILED(%02X)]           - AddNodeToNetwork failed, stopping... (Nodeinformation length %u)\r\n", bStatus, bLen);
    api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
    break;
    
  default:
    /* If were not pending, we want to turn off learn mode.. */
    mainlog_wr("ANN: [%02X]                        - Undefined status (Nodeinformation length %u)\r\n", bStatus, bLen);
    api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
    break;
  }
}


/*================================ SendData_Compl ============================
** Function description
**      Called when a SendData has been executed
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
SendData_Compl(
               BYTE bTxStatus, 
               WORD wTime)
{
  if (!bTxStatus)
  {
    mainlog_wr("SendData : Success... [%02X] TxTime = %ums\r\n", bTxStatus, wTime * 10);
  }
  else
  {
    mainlog_wr("SendData : Failure... [%02X] TxTime = %ums\r\n", bTxStatus, wTime * 10);
  }
}


/*======================== RequestNodeNeighborUpdate_Compl ===================
** Function description
**   ZW_RequestNodeNeighborUpdate callback
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
RequestNodeNeighborUpdate_Compl(
                                BYTE bStatus) /*IN Status of neighbor update process*/
{
  mainlog_wr("RequestNodeNeighborUpdate_Compl [%03u]: %02X\r\n", bNodeID, bStatus);
}


/*========================= RequestNodeInfo_Compl ============================
** Function description
**   ZW_RequestNodeInfo callback called when the request transmission has 
** been tried.
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
RequestNodeInfo_Compl(
                      BYTE bTxStatus)
{
  if (!bTxStatus)
  {
    mainlog_wr("Request Node Info transmitted successfully for node %u\r\n", bNodeID);
  }
  else
  {
    mainlog_wr("Request Node Info transmit failed for node %u\r\n", bNodeID);
  }
}


/*============================   SetDefault_Compl ============================
** Function description
**      Callback function called when ZW_SetDefault is done.
** Side effects:
**--------------------------------------------------------------------------*/
void
SetDefault_Compl(void)
{
  
  response_Upper(6, 1, 0);
  mainlog_wr("ZW_SetDefault done...\r\n");
}


/*==========================   SetLearnMode_Compl ============================
** Function description
**      Callback function called by protocol when in Receive mode
** Side effects:
**--------------------------------------------------------------------------*/
void
SetLearnMode_Compl(
                   BYTE bStatus,
                   BYTE bSource,
                   BYTE *pCmd,
                   BYTE bLen)
{
  mainlog_wr("ZW_SetLearnMode callback status %02X, source %02X, len %02X\r\n", bStatus, bSource, bLen);
  switch (bStatus)
  {
  case LEARN_MODE_STARTED:
    mainlog_wr("ZW_SetLearnMode LEARN_MODE_STARTED\r\n");
    learnMode_status = true;
    break;
    
  case LEARN_MODE_DONE:
    mainlog_wr("ZW_SetLearnMode LEARN_MODE_DONE\r\n");
    learnMode_status = false;
    break;
    
  case LEARN_MODE_FAILED:
    mainlog_wr("ZW_SetLearnMode LEARN_MODE_FAILED\r\n");
    learnMode_status = false;
    break;
    
  case LEARN_MODE_DELETED:
    mainlog_wr("ZW_SetLearnMode LEARN_MODE_DELETED\r\n");
    learnMode_status = false;
    break;
    
  default:
    mainlog_wr("ZW_SetLearnMode Unknown status\r\n");
    api.ZW_SetLearnMode(false, SetLearnMode_Compl);
    break;
  }
}


/*========================   RemoveFailedNode_Compl ==========================
** Function description
**      Callback function called by protocol when ZW_RemoveFailedNode has started
** Side effects:
**--------------------------------------------------------------------------*/
void
RemoveFailedNode_Compl(
                       BYTE bStatus)
{
  mainlog_wr("ZW_RemoveFailedNode callback status %02X\r\n", bStatus);
  switch (bStatus)
  {
  case ZW_NODE_OK:
    mainlog_wr("Failed node %03d not removed	- ZW_NODE_OK\r\n", bNodeID);
    break;
    
  case ZW_FAILED_NODE_REMOVED:
    mainlog_wr("Failed node %03d removed		- ZW_FAILED_NODE_REMOVED\r\n", bNodeID);
    break;
    
  case ZW_FAILED_NODE_NOT_REMOVED:
    mainlog_wr("Failed node %03d not removed	- ZW_FAILED_NODE_NOT_REMOVED\r\n", bNodeID);
    break;
    
  default:
    mainlog_wr("ZW_RemoveFailedNode unknown status %d\r\n", bStatus);
    break;
  }
}


/*==============================   GetControllerCapabilityStr ================
** Function description
**      Returns pointer to string describing the attached Controller device
**		and updates the global bControllerCapabilities variable accordingly
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
char *
GetControllerCapabilityStr(void)
{
  if (api.ZW_GetControllerCapabilities(&bControllerCapabilities))
  {
    if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT)
    {
      if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SUC)
      {
        sprintf_s(bCharBuf, sizeof(bCharBuf), "SIS");
      }
      else
      {
        sprintf_s(bCharBuf, sizeof(bCharBuf), "Inclusion");
      }
    }
    else
    {
      if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SUC)
      {
        sprintf_s(bCharBuf, sizeof(bCharBuf), "SUC");
      }
      else
      {
        if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_SECONDARY)
        {
          sprintf_s(bCharBuf, sizeof(bCharBuf), "Secondary");
        }
        else
        {
          sprintf_s(bCharBuf, sizeof(bCharBuf), "Primary");
        }
      }
    }
    sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " Controller");
    if (0 < MySystemSUCID)
    {
      if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NODEID_SERVER_PRESENT)
      {
        sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " in a SIS network");
      }
      else
      {
        sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), " in a SUC network");
      }
    }
    if (bControllerCapabilities & CONTROLLER_CAPABILITIES_IS_REAL_PRIMARY)
    {
      sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (Real primary)");
    }
    if (bControllerCapabilities & CONTROLLER_CAPABILITIES_ON_OTHER_NETWORK)
    {
      sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (Other network)");
    }
    if (bControllerCapabilities & CONTROLLER_CAPABILITIES_NO_NODES_INCUDED)
    {
      sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf), ", (No nodes)");
    }
  }
  else
  {
    sprintf_s(bCharBuf, sizeof(bCharBuf), "Controller Could not determine Controller Capabilities");
  }
  return bCharBuf;
}


/*============================== ReloadNodeList =============================
** Function description
**      Reloads the node IDs and types from the Device module
**		if fInitializeNetworkManagement = true then the NetworkManagement_Init
** Side effects:
**		Reinitializes the nodeType, nodeList and
**		the abListeningNodeList arrays
**
**--------------------------------------------------------------------------*/
BOOL 
ReloadNodeList(
               BOOL fInitializeNetworkManagement)
{
  BOOL bFound = FALSE;
  BYTE byNode = 1;
  BYTE j,i;
  NODEINFO nodeInfo;
  bMaxNodeID = 0;
  nodeListSize = 0;
  bListeningNodeListSize = 0;
  bNetworkRepeaterCount = 0;
  api.MemoryGetID(MySystemHomeId, &MyNodeId);
  MySystemSUCID = api.ZW_GetSUCNodeID();
  mainlog_wr("SUC ID %03u\r\n", MySystemSUCID);
  mainlog_wr("Device HomeID %02X%02X%02X%02X, NodeID %03u\r\n",
             MySystemHomeId[0], MySystemHomeId[1], MySystemHomeId[2], MySystemHomeId[3], MyNodeId);
  memset(bNodeExistMask, 0, sizeof(bNodeExistMask));
  api.SerialAPI_GetInitData(&bSerialAPIVer, &bDeviceCapabilities, &bNodeExistMaskLen, bNodeExistMask);
  if (bDeviceCapabilities & GET_INIT_DATA_FLAG_SLAVE_API)
  {
    mainlog_wr("Device is Slave\r\n");
  }
  else
  {
    mainlog_wr("Device is %s\r\n", GetControllerCapabilityStr());
  }
  /* Reset the nodelists before loading them again */
  memset(nodeType, 0, sizeof(nodeType));
  memset(nodeList, 0, sizeof(nodeList));
  memset(abListeningNodeList, 0, sizeof(abListeningNodeList));
  memset(afIsNodeAFLiRS, 0, sizeof(afIsNodeAFLiRS));
  for (i = 0; i < bNodeExistMaskLen; i++)
  {
    if (bNodeExistMask[i] != 0)
    {
      bFound = TRUE;
      for (j = 0; j < 8; j++)
      {
        if (bNodeExistMask[i] & (1 << j))
        {
          api.ZW_GetNodeProtocolInfo(byNode, &nodeInfo);
          memcpy(&sNetworkManagementUT.nodeDescriptor[byNode - 1].nodeInfo, &nodeInfo, sizeof(nodeInfo));
          nodeType[byNode] = nodeInfo.nodeType.generic;
          nodeList[nodeListSize++] = byNode;
          /* We want listening nodes and FLiRS nodes in networkTest list */
          if (byNode != MyNodeId)
          {
            if ((nodeInfo.capability & NODEINFO_LISTENING_SUPPORT) ||
                (0 != (nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250))))
            {
              /* Node is either a AC powered Listening node or a Beam wakeup Node */
              abListeningNodeList[bListeningNodeListSize++] = byNode;
              if (0 == (nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250)))
              {
                if (nodeInfo.capability & NODEINFO_ROUTING_SUPPORT)
                {
                  /* One more repeater - AC powered Listening nodes are counted as Repeater */
                  bNetworkRepeaterCount++;
                }
              }
              else
              {
                afIsNodeAFLiRS[byNode - 1] = true;
              }
            }
          }
          else
          {
          }
          mainlog_wr("Node %03u %s %02X %s\r\n",
                     byNode, (nodeInfo.capability & NODEINFO_LISTENING_SUPPORT) ? "TRUE " : "FALSE", 
                     nodeInfo.security & (NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_1000 | NODEINFO_ZWAVE_SENSOR_MODE_WAKEUP_250), 
                     WriteNodeTypeString(nodeType[byNode]));
          bMaxNodeID = byNode;
        }
        byNode++;
      }
    }
    else
    {
      byNode += 8;
    }
  }
  mainlog_wr("Listening nodes registered %u\r\n", bListeningNodeListSize);
  
  if (fInitializeNetworkManagement)
  {
    InitializeSerialAPICapabilities();
    if (!NetworkManagement_Init(&sController_SerialAPI_Capabilities, abListeningNodeList, bListeningNodeListSize, afIsNodeAFLiRS, bNetworkRepeaterCount, &sNetworkManagementUT))
    {
      mainlog_wr("NetworkManagement_Init: Essential functionality NOT supported\r\n");
    }
    else
    {
      mainlog_wr("NetworkManagement_Init: Essential functionality supported\r\n");
    }
  }
  return bFound;
}


/*============================= CommandClassName =============================
** Function description
**      Convert Command Class identifier to char * strings
** Side effects:
**
**--------------------------------------------------------------------------*/
char 
*CommandClassName(
                  BYTE bCmdClass)
{
  char *retStr = "Unknown Command Class";
  
  switch (bCmdClass)
  {
  case COMMAND_CLASS_BASIC:
    retStr = "COMMAND_CLASS_BASIC";
    break;
    
  case COMMAND_CLASS_ALARM:
    retStr = "COMMAND_CLASS_ALARM";
    break;
    
  case COMMAND_CLASS_POWERLEVEL:
    retStr = "COMMAND_CLASS_POWERLEVEL";
    break;
    
  case COMMAND_CLASS_ASSOCIATION:
    retStr = "COMMAND_CLASS_ASSOCIATION";
    break;
    
  case COMMAND_CLASS_CONFIGURATION:
    retStr = "COMMAND_CLASS_CONFIGURATION";
    break;
    
  case COMMAND_CLASS_CONTROLLER_REPLICATION:
    retStr = "COMMAND_CLASS_CONTROLLER_REPLICATION";
    break;
    
  case COMMAND_CLASS_CRC_16_ENCAP:
    retStr = "COMMAND_CLASS_CRC_16_ENCAP";
    break;
    
  case COMMAND_CLASS_DOOR_LOCK:
    retStr = "COMMAND_CLASS_DOOR_LOCK";
    break;
    
  case COMMAND_CLASS_HAIL:
    retStr = "COMMAND_CLASS_HAIL";
    break;
    
  case COMMAND_CLASS_LOCK:
    retStr = "COMMAND_CLASS_LOCK";
    break;
    
  case COMMAND_CLASS_MANUFACTURER_SPECIFIC:
    retStr = "COMMAND_CLASS_MANUFACTURER_SPECIFIC";
    break;
    
  case COMMAND_CLASS_PROTECTION:
    retStr = "COMMAND_CLASS_PROTECTION";
    break;
    
  case COMMAND_CLASS_METER:
    retStr = "COMMAND_CLASS_METER";
    break;
    
  case COMMAND_CLASS_MULTI_CHANNEL_V2:
    retStr = "COMMAND_CLASS_MULTI_CHANNEL/COMMAND_CLASS_MULTI_INSTANCE";
    break;
    
  case COMMAND_CLASS_SIMPLE_AV_CONTROL:
    retStr = "COMMAND_CLASS_SIMPLE_AV_CONTROL";
    break;
    
  case COMMAND_CLASS_SWITCH_ALL:
    retStr = "COMMAND_CLASS_SWITCH_ALL";
    break;
    
  case COMMAND_CLASS_SWITCH_BINARY:
    retStr = "COMMAND_CLASS_SWITCH_BINARY";
    break;
    
  case COMMAND_CLASS_SWITCH_MULTILEVEL:
    retStr = "COMMAND_CLASS_SWITCH_MULTILEVEL";
    break;
    
  case COMMAND_CLASS_VERSION:
    retStr = "COMMAND_CLASS_VERSION";
    break;
    
  case COMMAND_CLASS_WAKE_UP:
    retStr = "COMMAND_CLASS_WAKE_UP";
    break;
    
  case COMMAND_CLASS_ZIP_6LOWPAN:
    retStr = "COMMAND_CLASS_ZIP_6LOWPAN";
    break;
    
  case COMMAND_CLASS_ZIP:
    retStr = "COMMAND_CLASS_ZIP";
    break;
    
  default:
    break;
  }
  return retStr;
}


/*=============================== DumpNodeInfo ===============================
** Function description
**      Dump the registered nodeinformation for bNodeID to log and screen
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DumpNodeInfo(
             BYTE bNodeID)
{
  int indx = bNodeID - 1;
  
  if ((0 < bNodeID) && (ZW_MAX_NODES >= bNodeID))
  {
    mainlog_wr("Nodeinformation registered for nodeID %03d\r\n", bNodeID);
    mainlog_wr("capability %02X, security %02X, reserved %02X, nodeType Basic %02X, Generic %02X, Specific %02X\r\n",
               sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.capability, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.security, 
               sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.reserved, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.basic, 
               sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.generic, sNetworkManagementUT.nodeDescriptor[indx].nodeInfo.nodeType.specific);
    mainlog_wr("Command Classes registered as supported for nodeID %03d\r\n", bNodeID);
    for (int n = 0; 0 < sNetworkManagementUT.nodeDescriptor[indx].cmdClasses[n]; n++)
    {
      mainlog_wr("%s", CommandClassName(sNetworkManagementUT.nodeDescriptor[indx].cmdClasses[n]));
    }
  }
  else
  {
    mainlog_wr("DumpNodeInfo nodeid not valid %03d\r\n", bNodeID);
  }
}



/*========================== RequestNetworkUpdate_Compl ======================
** Function description
**      Callback function called by RequestNetworkUpdate functionality when done
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNetworkUpdate_Compl(
                           BYTE bStatus)
{
  mainlog_wr("ZW_RequestNetworkUpdate status - %d\r\n", bStatus);
}


/*========================= NetworkRediscoveryComplete =======================
** Function description
**      Callback function called when Network RediscoveryFunctionality has
** stopped.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkRediscoveryComplete(
                           BYTE bStatus)
{
  testStarted = false;
  mainlog_wr("Network Rediscovery stopped - Status %u\r\n", bStatus);
}


/*========================== RequestNextNodeInformation ======================
** Function description
**      Callback function called if timeout waiting for Nodeinformation frame
** requested with DoRequestnodeInformation.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNextNodeInformation(int H)
{
  bool retVal = false;
  int i;
  timerH = 0; //callback form timeout. Reset handle
  
  for (i = sNetworkManagementUT.bNHSCurrentIndex + 1; i < bListeningNodeListSize; i++)
  {
    /* If first Command Class is NONE ZERO then we do have Command Class information for Node */
    if (0 == sNetworkManagementUT.nodeDescriptor[abListeningNodeList[i] - 1].cmdClasses[0])
    {
      /* We need to request nodeinformation from at least this node */
      retVal = true;
      break;
    }
  }
  /* Are we done with NodeInformation Requests */
  if (true == retVal)
  {
    DoRequestForNodeInformation(i);
  }
  else
  {
    sNetworkManagementUT.bNHSCurrentIndex = 0;
    /* We got all the nodeinformation so start the real Network Health Test */
    testStarted = NetworkHealthStart();
    if (!testStarted)
    {
      mainlog_wr("NetworkHealthStart Failed to start Network Health Test\r\n");
    }
  }
}


/*========================== DoNetworkHealthMaintenance ======================
** Function description
**		Does the actual Maintenance update loop.
** requested with DoRequestnodeInformation.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DoNetworkHealthMaintenance()
{
  sNetworkManagementUT.bTestMode = MAINTENANCE;
  
}


/*=========================== NetworkHealthComplete ==========================
** Function description
**      Callback function called when Network Health Functionality has stopped.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
NetworkHealthComplete(
                      BYTE bStatus)
{
  testStarted = false;
  mainlog_wr("Network Health stopped - Status %u\r\n", bStatus);
}


/*=========================== NetworkHealthComplete ==========================
** Function description
**      Function called starting the FULL or SINGLE Network Health Test
** Side effects:
**
**--------------------------------------------------------------------------*/
bool
NetworkHealthStart()
{
  bool retVal = false;
  /* We got all the nodeinformation so start the real Network Health Test */
  switch (sNetworkManagementUT.bTestMode)
  {
  case FULL:
    {
      retVal = NetworkManagement_NetworkHealth_Start(NetworkHealthComplete);
    }
    break;
    
  case SINGLE:
    {
      sNetworkManagementUT.bCurrentTestNodeID = bNodeID;
      retVal = NetworkManagement_NetworkHealth_Start(NetworkHealthComplete);
    }
    break;
    
  case MAINTENANCE:
    {
    }
    break;
    
  default:
    break;
  }
  return retVal;
}


/*===================== RequestNodeInformation_Completed =====================
** Function description
**      Callback function called when the ZW_RequestNodeInfo call started by
** DoRequestForNodeInformation has been executed
** Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestNodeInformation_Completed(
                                 BYTE bTxStatus)
{
  if (!bTxStatus)
  {
    mainlog_wr("Request Node Info transmitted successfully to node %03d\r\n", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
  }
  else
  {
    mainlog_wr("Request Node Info transmit failed to node %03d\r\n", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
  }
}


/* Set Request Nodeinformation timeout to 8 seconds */
#define TIMEOUT_NODEINFORMATION		8

/*======================= DoRequestForNodeInformation ========================
** Function description
**      Function to update 
** Side effects:
**
**--------------------------------------------------------------------------*/
void
DoRequestForNodeInformation(
                            BYTE bNextIndex)
{
  sNetworkManagementUT.bNHSCurrentIndex = bNextIndex;
  pTimer = CTime::GetInstance();
  //If timer is running, kill it!
  if (0 != timerH)
  {
    pTimer->Kill(timerH);
  }
  timerH = pTimer->Timeout(TIMEOUT_NODEINFORMATION, RequestNextNodeInformation);
  if (0 == timerH)
  {
    mainlog_wr("DoRequestForNodeInformation no timer! ERROR!\r\n");
    /* Go on and test anyway */
    if (!NetworkHealthStart())
    {
      mainlog_wr("Network Health Test could not be started - FAILED\r\n");
      /* Could not start NetworkHealth */
      NetworkHealthComplete(0);
    }
  }
  else
  {
    osDelay(10);
    if (api.ZW_RequestNodeInfo(abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex], RequestNodeInformation_Completed))
    {
      mainlog_wr("ZW_RequestNodeInfo has been initiated for node %u\r\n", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
    }
    else
    {
      mainlog_wr("ZW_RequestNodeInfo could not be initiated for node %u\r\n", abListeningNodeList[sNetworkManagementUT.bNHSCurrentIndex]);
      mainlog_wr("Timeout callback function will initiate and try with next node\r\n");
    }
  }
}


/*=============================== NetworkHealth ==============================
** Function description
**      Function called when starting either FULL or SINGLE Network Health 
** test. First is determined if any node needs to be requested for 
** nodeinformation which includes command classes supported. If so then
** this is then initiated. The Network Health Test is then started if no node
** needs to inform Controller about Command Classes supported or when all
** nodes needing it has been queried
** Side effects:
**
**--------------------------------------------------------------------------*/
bool
NetworkHealth(
              eTESTMODE bTestMode)
{
  bool retVal = false;
  int i;
  sNetworkManagementUT.bTestMode = bTestMode;
  for (i = 0; i < bListeningNodeListSize; i++)
  {
    /* If first Command Class is NONE ZERO then we do have Command Class information for Node */
    if (0 == sNetworkManagementUT.nodeDescriptor[abListeningNodeList[i] - 1].cmdClasses[0])
    {
      /* We need to request nodeinformation from at least this node */
      retVal = true;
      break;
    }
  }
  /* Do we need to request NodeInformation from Nodes Under Test */
  if (true == retVal)
  {
    DoRequestForNodeInformation(i);
  }
  else
  {
    /* Start the real Network Health Test */
    retVal = NetworkHealthStart();
  }
  return retVal;
}


/*==================== CB_ToggleBasicSetONOFF_Completed ======================
** Function description
**      Callback function called when NetworkManagement_ZW_SendData has been 
** executed after selecting Network Health Menuitem '5'.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_ToggleBasicSetONOFF_Completed(
                                 BYTE bTxStatus, 
                                 WORD wTime)
{
  mainlog_wr("Maintain  - Node %03u - BASIC SET %s %s (%ums)\r\n",
             bNodeID, (255 == pBasicBuf[2]) ? " ON" : "OFF", 
             (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", wTime * 10);
}


/*============================ CB_PingTestComplete ===========================
** Function description
**      Callback function called when Ping test transmit has been executed.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_PingTestComplete(
                    BYTE bTxStatus, 
                    WORD wTime)
{
  mainlog_wr("Ping Done - Node %03u - %s - Latency %ums\r\n", abListeningNodeList[bPingNodeIndex],
             (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", wTime * 10);
  if (TRANSMIT_COMPLETE_OK != bTxStatus)
  {
    abPingFailed[abPingFailedSize++] = abListeningNodeList[bPingNodeIndex];
  }
  if (++bPingNodeIndex < bListeningNodeListSize)
  {
    testStarted = NetworkManagement_NH_TestConnection(abListeningNodeList[bPingNodeIndex], CB_PingTestComplete);
    if (testStarted)
    {
      mainlog_wr("Ping      - Node %03u\r\n", abListeningNodeList[bPingNodeIndex]);
    }
    else
    {
      mainlog_wr("Ping Node - could not be started - stopping\r\n");
    }
  }
  else
  {
    testStarted = false;
  }
  if (!testStarted)
  {
    if (0 < abPingFailedSize)
    {
      char bCharBuf[1024];
      
      sprintf_s(bCharBuf, sizeof(bCharBuf), "Ping Failed - ");
      for (int i = 0; i < abPingFailedSize; i++)
      {
        sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf),"%0X ", abPingFailed[i]);
      }
      mainlog_wr(bCharBuf);
    }
    else
    {
      mainlog_wr("Ping Success - All Nodes answered\r\n");
    }
    mainlog_wr("Ping Node - End\r\n");
    testStarted = false;
  }
}


/*========================= CB_GetVersionTestComplete ========================
** Function description
**      Callback function called when Get Version transmit has been executed.
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_GetVersionTestComplete(
                          BYTE bTxStatus, 
                          WORD wTime)
{
  /* If timer is running, kill it! */
  if (0 != timerGetVersionReportH)
  {
    pGetVersionReportTimer->Kill(timerGetVersionReportH);
  }
  timerGetVersionReportH = pGetVersionReportTimer->Timeout((TRANSMIT_COMPLETE_OK == bTxStatus) ? 
GETVERSIONREPORTTIMEOUT :
                                                              GETVERSIONREPORTTIMEOUT / 2,
                                                              GetVersionReportTimeout);
  mainlog_wr("Get Version Done - Node %03u - %s - Latency %ums\r\n", abListeningNodeList[bPingNodeIndex],
             (TRANSMIT_COMPLETE_OK == bTxStatus) ? "SUCCESS" : "FAILED", wTime * 10);
  if (TRANSMIT_COMPLETE_OK != bTxStatus)
  {
    abPingFailed[abPingFailedSize++] = abListeningNodeList[bPingNodeIndex];
  }
}


/*========================== GetVersionReportTimeout =========================
** Function description
**      Get Version Report Timeout functionality
**		
** Side effects:
**		
**--------------------------------------------------------------------------*/
void
GetVersionReportTimeout(
                        int H)
{
  /* Make sure timer handle is ZERO indicating not running */
  timerGetVersionReportH = 0;
  if (++bPingNodeIndex < bListeningNodeListSize)
  {
    testStarted = NetworkManagement_ZW_SendData(abListeningNodeList[bPingNodeIndex], pBasicBuf, 2, CB_GetVersionTestComplete);
    if (testStarted)
    {
      mainlog_wr("Get Version - Node %03u\r\n", abListeningNodeList[bPingNodeIndex]);
    }
    else
    {
      mainlog_wr("Get Version - could not be started - stopping\r\n");
    }
  }
  else
  {
    testStarted = false;
  }
  if (!testStarted)
  {
    if (0 < abPingFailedSize)
    {
      char bCharBuf[1024];
      
      sprintf_s(bCharBuf, sizeof(bCharBuf), "Get Version - ");
      for (int i = 0; i < abPingFailedSize; i++)
      {
        sprintf_s(&bCharBuf[strlen(bCharBuf)], sizeof(bCharBuf) - strlen(bCharBuf),"%0X ", abPingFailed[i]);
      }
      mainlog_wr(bCharBuf);
    }
    else
    {
      mainlog_wr("Get Version Success - All Nodes answered\r\n");
    }
    mainlog_wr("Get Version Node - End\r\n");
    testStarted = false;
  }
}


/*========================== CB_InjectNewLWR_Compl ===========================
** Function description
**      Callback function called when api.MemoryPutBuffer has finished
** Side effects:
**
**--------------------------------------------------------------------------*/
void
CB_InjectNewLWR_Compl(void)
{
  mainlog_wr("New LWR has been injected for %03u\r\n", bNodeID);
}


/*========================== EnterNewCurrentNodeID ===========================
** Function description
**      Function used to change the 'current NodeID'
** Side effects:
**
**--------------------------------------------------------------------------*/
void 
EnterNewCurrentNodeID()
{
  char instr[10];
  
  printf("Enter new current NodeID:\r\n ");
  
  UARTgetsNonblocking(instr, sizeof(instr));
  
  bNodeID = atoi(instr);
  printf("Current NodeID is now %i\r\n", bNodeID);
}

/*****************************************************************************/
void	fAddNodeToNetwork()
{
  bAddRemoveNodeFromNetwork = !bAddRemoveNodeFromNetwork;
  if (bAddRemoveNodeFromNetwork)
  {
    api.ZW_AddNodeToNetwork(ADD_NODE_ANY|ADD_NODE_OPTION_HIGH_POWER|ADD_NODE_OPTION_NETWORK_WIDE, SetLearnNodeState_Compl);
    mainlog_wr("ZW_AddNodeToNetwork ADD_NODE_ANY|ADD_NODE_OPTION_NETWORK_WIDE\r\n");
  }
  else
  {
    api.ZW_AddNodeToNetwork(ADD_NODE_STOP, SetLearnNodeState_Compl);
    mainlog_wr("ZW_AddNodeToNetwork ADD_NODE_STOP\r\n");
  }
}


void	fRemoveNodeFromNetwork()
{
  bAddRemoveNodeFromNetwork = !bAddRemoveNodeFromNetwork;
  if (bAddRemoveNodeFromNetwork)
  {
    api.ZW_RemoveNodeFromNetwork(REMOVE_NODE_ANY, SetLearnNodeStateDelete_Compl);
    mainlog_wr("ZW_RemoveNodeFromNetwork REMOVE_NODE_ANY|ADD_NODE_OPTION_NETWORK_WIDE\r\n");
  }
  else
  {
    api.ZW_RemoveNodeFromNetwork(REMOVE_NODE_STOP, SetLearnNodeStateDelete_Compl);
    mainlog_wr("ZW_RemoveNodeFromNetwork REMOVE_NODE_STOP\r\n");
  }
}

void	fRemoveFailedNode()
{
  BYTE abFuncIDToSupport[] = {FUNC_ID_ZW_REMOVE_FAILED_NODE_ID};
  if (NetworkManagement_IsFuncIDsSupported(abFuncIDToSupport, sizeof(abFuncIDToSupport)))
  {
    mainlog_wr("ZW_RemoveFailedNode supported by Z-Wave SerialAPI module\r\n");
    mainlog_wr("ZW_RemoveFailedNode %03d\r\n", bNodeID);
    BYTE retVal = api.ZW_RemoveFailedNode(bNodeID, RemoveFailedNode_Compl);
    {
      switch (retVal)
      {
      case ZW_FAILED_NODE_REMOVE_STARTED:
        {
          mainlog_wr("ZW_RemoveFailedNode started on node %03d\r\n", bNodeID);
        }
        break;
        
      case ZW_NOT_PRIMARY_CONTROLLER:
        {
          mainlog_wr("ZW_RemoveFailedNode - ZW_NOT_PRIMARY_CONTROLLER\r\n");
        }
        break;
        
      case ZW_NO_CALLBACK_FUNCTION:
        {
          mainlog_wr("ZW_RemoveFailedNode - ZW_NO_CALLBACK_FUNCTION\r\n");
        }
        break;
        
      case ZW_FAILED_NODE_NOT_FOUND:
        {
          mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_NOT_FOUND\r\n");
        }
        break;
        
      case ZW_FAILED_NODE_REMOVE_PROCESS_BUSY:
        {
          mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_REMOVE_PROCESS_BUSY\r\n");
        }
        break;
        
      case ZW_FAILED_NODE_REMOVE_FAIL:
        {
          mainlog_wr("ZW_RemoveFailedNode - ZW_FAILED_NODE_REMOVE_FAIL\r\n");
        }
        break;
        
      default:
        {
          mainlog_wr("ZW_RemoveFailedNode failed - %d\r\n", retVal);
        }
        break;
      }
    }
  }
  else
  {
    mainlog_wr("ZW_RemoveFailedNode Not supported by Z-Wave SerialAPI module\r\n");
  }
}

void	fSetLearnMode()
{
  if (!learnMode_status)
  {
    api.ZW_SetLearnMode(true, SetLearnMode_Compl);
    mainlog_wr("ZW_SetLearnMode enabling learnMode/receive\r\n");
  }
  else
  {
    api.ZW_SetLearnMode(false, SetLearnMode_Compl);
    mainlog_wr("ZW_SetLearnMode disabling learnMode/receive\r\n");
  }
  learnMode_status = !learnMode_status;
}

void	fSendData(BYTE Control_from, BYTE SetData)
{
  static BYTE pData[3];
  pData[0] = 32;
  pData[1] = 1;
  if (Control_from == 0)	//Console
  {
    if (!pData[2])
    {
      pData[2] = 255;
    }
    else
    {
      pData[2] = 0;
    }
  }
  else		//BT ot USB
  {
    if (SetData)
      pData[2] = 255;
    else
      pData[2] = 0;
  }
  mainlog_wr("ZW_SendData Basic Set %02X, NodeID %03d - \r\n", (unsigned int)pData[2], bNodeID);
  api.ZW_SendData(bNodeID, pData, 3, TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE, SendData_Compl);
  mainlog_wr("Current NodeID now %03d\r\n", bNodeID);
}

void	fVersion()

{
  int i;
  BYTE buf[20];
  BYTE libType = api.ZW_Version(buf);
  for (i = 0; i < 15; i++)
  {
    if (buf[i] == 0)
    {
      break;
    }
  }
  if (buf[i] != 0)
  {
    buf[i] = 0;
  }
  char *pLibTypeStr = GetValidLibTypeStr(libType);
  mainlog_wr("Z-Wave %s protocol Version %s\r\n", pLibTypeStr, buf);
}

void	fGetCapabilities()
{
  BYTE pBuf[64];
  char pCharBuf[256];
  int bFuncCmd = 0;
  mainlog_wr("Calling Serial_Get_Capabilities\r\n");
  api.SerialAPI_Get_Capabilities(pBuf);
  mainlog_wr("SERIAL_APPL_VERSION %02x, SERIAL_APPL_REVISION %02x\r\n", pBuf[0], pBuf[1]);
  mainlog_wr("SERIALAPI_MANUFACTURER_ID1 %02x, SERIALAPI_MANUFACTURER_ID2 %02x\r\n", pBuf[2], pBuf[3]);
  mainlog_wr("SERIALAPI_MANUFACTURER_PRODUCT_TYPE1 %02x, SERIALAPI_MANUFACTURER_PRODUCT_TYPE2 %02x\r\n", pBuf[4], pBuf[5]);
  mainlog_wr("SERIALAPI_MANUFACTURER_PRODUCT_ID1 %02x, SERIALAPI_MANUFACTURER_PRODUCT_ID2 %02x\r\n", pBuf[6], pBuf[7]);
  mainlog_wr("FUNCID_SUPPORTED_BITMASK[]\r\n");
  for (int i = 8; i < 40; i++)
  {
    char cch = ' ';
    sprintf_s(pCharBuf, sizeof(pCharBuf), "%02x [", pBuf[i]);
    for (int j = 1, mask = 1; j <= 8; j++, mask <<= 1)
    {
      if (mask & pBuf[i])
      {
        if (cch != ' ')
        {
          sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf),"%c", cch);
        }
        sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf)-strlen(pCharBuf), "%02X", bFuncCmd + j);
        cch = ',';
      }
    }
    sprintf_s(&pCharBuf[strlen(pCharBuf)], sizeof(pCharBuf) - strlen(pCharBuf), "]");
    mainlog_wr("%s\r\n", pCharBuf);
    bFuncCmd += 8;
  }
}

void 	fReloadNodeList()
{
  int numberOfNodes = 0;
  int	idx;
  
  if (Control_from == 1)
  {
    UpperRsp[1] = 2;
    idx = 2;
  }
  else
  {
    UpperRsp[0] = 0xAB;
    UpperRsp[1] = 0xAD;
    UpperRsp[2] = 02;
    idx = 3;
  }
  mainlog_wr("Initialize nodeList with node info from device module\r\n");
  ReloadNodeList(false);
  for (int i = 0; i < ZW_MAX_NODES; i++)
  {
    if (nodeType[i])
    {
      UpperRsp[idx + numberOfNodes * 3 ] = i;
      UpperRsp[idx + numberOfNodes * 3 + 1] = nodeType[i];
      UpperRsp[idx + numberOfNodes * 3 + 2] = 0;
      
      numberOfNodes++;
      mainlog_wr("Node %03u - %s\r\n", i, WriteNodeTypeString(nodeType[i]));
    }
  }
  
  
  mainlog_wr("Total of %03u nodes in network\r\n", numberOfNodes);
  
  if (Control_from == 1)
  {
    UpperRsp[0] = (2 + numberOfNodes * 3);
    put_BTresponse(UpperRsp, (2 + numberOfNodes * 3) );
    printf ("Response Info to Upper(%d) [%02X][L=%d]....\r\n", Control_from, 02, (2 + numberOfNodes * 3));
  }
  else if (Control_from == 2)
  {
    put_USBresponse(UpperRsp, (3 + numberOfNodes * 3));
    printf ("Response Info to Upper(%d) [%02X][L=%d]....\r\n", Control_from, 02, (3 + numberOfNodes * 3));
  }
}


////////////////////////////////////////////////////////////////

#if	0
void get_NodeInfomation(BYTE *get_nodeList, BYTE *get_nodeType, BYTE *NumofNode)
{
  get_nodeList = nodeList;
  get_nodeType = nodeType;
  * NumofNode = ZW_MAX_NODES;
}
#endif

/*==================================== main ==================================
** Function description
**      main
** Side effects:
**
**--------------------------------------------------------------------------*/

void ZWApp_main(void const * argument)
{
  BYTE uCversion[100];
  BYTE libType;
  char instr[10];
  char portstr[16];
  char ch = 0, i = 0;
  BYTE UpperCmd[32];
  BYTE UpperRsp[32];
  BYTE UpperLen;
  
  BYTE Control_Data = 0;
  
  osDelay(500);
  mainlog_wr("ZW_App(SHACS) - System started\r\n");
  mainlog_wr("Using serial port %d\r\n", ZW_UART_PORT);
  if (api.Initialize(ZW_UART_PORT, CSerialAPI::SPEED_115200, ApplicationCommandHandler, ApplicationCommandHandler_Bridge, CommErrorNotification))
  {
    
    char *pLibTypeStr;
    libType = api.ZW_Version((BYTE*)uCversion);	//Get the lib type and version from module
    pLibTypeStr = GetValidLibTypeStr(libType);
    if (0 != strcmp("", pLibTypeStr))
    {
      mainlog_wr("Z-Wave %s based serial API found\r\n", pLibTypeStr);
      printf("%s - %s", uCversion, pLibTypeStr);
    }
    else
    {
      mainlog_wr("No Serial API module detected, [libtype %u]- check serial connections.\r\nThis application requires a Serial API\r\non a Z-Wave module connected via a serialport.\r\nDownload the correct Serial API code to the Z-Wave Module and restart the application.", libType);
      libType = 0;  /* Indicate we want to end this */
      
    }
    
    osTimerStart(ZWhostAppTimerHandle, 1000L);
    printf("ZWhostAppTimer start \r\n");
    
    if (libType)
    {
      static int sendState = 0;
      
      mainlog_wr("%s\r\n", uCversion);
      memset(sNetworkManagementUT.nodeDescriptor, 0, sizeof(sNetworkManagementUT.nodeDescriptor));
      
      /*Set Idle learn mode function.*/
      api.ZW_SetIdleNodeLearn(IdleLearnNodeState_Compl);
#if	1
      ReloadNodeList(false);
#else
      ReloadNodeList(true);
#endif
      if (bNodeID == MyNodeId)
      {
        bNodeID++;
      }
      printf("Input CMD for ZW Module\r\n");
      //            while ((ch != 'Q') || (ch != 'q'))
      while (1)
      {
        ch = 0;
        if (get_BTcommand(UpperCmd, &UpperLen) != 0)
        {
          printf("Get BT Cmd\r\n");
          printf("Len=%d [%02X] [%02X] [%02X]\r\n", UpperLen, UpperCmd[0], UpperCmd[1], UpperCmd[2]);
          switch (UpperCmd[1] )
          {
          case 2:
            ch ='I';
            break;
          case 3:		//Node On/Off
            bNodeID = UpperCmd[2];
            Control_Data = UpperCmd[3];
            printf("Cmd=%d, NodeId = %d SetVal = %d\r\n", UpperCmd[1], bNodeID, Control_Data);
            ch = '*';
            break;
          case 4:		//Add Node
            bNodeID = UpperCmd[2];
            Control_Data = UpperCmd[3];
            printf("Cmd=%d, Add node\r\n", UpperCmd[1]);
            ch = 'A';
            break;
          case 5:		//Query Stats
            bNodeID = UpperCmd[2];
            printf("Cmd=%d, Query Node[%d] status\r\n", UpperCmd[1], bNodeID);
            break;
          case 6:		//Reset Default
            printf("Cmd=%d, Reset to Default\r\n", UpperCmd[1]);
            ch = 'D';
            break;
          case 7:		//Query Stats
            printf("Cmd=%d, Set OTG\r\n", UpperCmd[1]);
            if (UpperCmd[2] == 0)
            {
              printf("\r\nUsb OTG Disable\r\n");
#if 1
              GPIO_InitTypeDef GPIO_InitStruct;
              
              /*Configure GPIO pin : PB5 - Open Drain*/
              GPIO_InitStruct.Pin = GPIO_PIN_5;
              GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
              
              /*Configure GPIO pin : PB6 */
              GPIO_InitStruct.Pin = GPIO_PIN_6;
              GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState) GPIO_PIN_RESET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState) GPIO_PIN_SET);
            }
            else
            {
              
              printf("\r\nUsb OTG Enable\r\n");
#if 1
              GPIO_InitTypeDef GPIO_InitStruct;
              
              /*Configure GPIO pin : PB5 - Open Drain*/
              GPIO_InitStruct.Pin = GPIO_PIN_5;
              GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
              
              /*Configure GPIO pin : PB6 */
              GPIO_InitStruct.Pin = GPIO_PIN_6;
              GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#else
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState) GPIO_PIN_SET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState) GPIO_PIN_RESET);
#endif
            }
            Control_from = 1;
            response_Upper(0x07, 0x01, 00);
            break; 
          }
          Control_from = 1; 
        }
        else if (get_USBcommand(UpperCmd, &UpperLen) != 0)
        {
          printf("Get USB Cmd\r\n");
          printf("Len=%d [%02X] [%02X] [%02X]\r\n", UpperLen, UpperCmd[0], UpperCmd[2], UpperCmd[3]);
          switch (UpperCmd[2] )
          {
          case 2:
            ch = 'I';
            break;
          case 3:		//Node On/Off
            bNodeID = UpperCmd[3];
            Control_Data = UpperCmd[4];
            printf("Cmd=%d, NodeId = %d SetVal = %d\r\n", UpperCmd[3], bNodeID, Control_Data);
            ch = '*';
            break;
          case 4:		//Add Node
            bNodeID = UpperCmd[3];
            Control_Data = UpperCmd[4];
            printf("Cmd=%d, Add node\r\n", UpperCmd[1]);
            ch = 'A';
            break;
          case 5:		//Query Stats
            bNodeID = UpperCmd[3];
            printf("Cmd=%d, Query Node[%d] status\r\n", UpperCmd[2], bNodeID);
            break;
          case 6:		//Reset Default
            printf("Cmd=%d, Reset to Default\r\n", UpperCmd[2]);
            ch = 'D';
            break;
          case 7:		//Query Stats
            printf("Cmd=%d, Set OTG\r\n", UpperCmd[2]);
            if (UpperCmd[3] == 0)
            {
              printf("\r\nUsb OTG Disable\r\n");
              
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState)GPIO_PIN_RESET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState)GPIO_PIN_SET);
            }
            else
            {
              printf("\r\nUsb OTG Enable\r\n");
              
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState)GPIO_PIN_SET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState)GPIO_PIN_RESET);
            }
            Control_from = 2;
            response_Upper(0x07, 0x01, 00);
            break;
            
          }
          Control_from = 2;
        }
        else if(get_WIFIcommand(UpperCmd, &UpperLen) != 0)
        {
          printf("Get Wifi Cmd\r\n");
          printf("Len=%d [%02X] [%02X] [%02X]\r\n", UpperLen, UpperCmd[0], UpperCmd[1], UpperCmd[2]);
          switch (UpperCmd[1] )
          {
          case 2:
            ch ='I';
            break;
          case 3:		//Node On/Off
            bNodeID = UpperCmd[2];
            Control_Data = UpperCmd[3];
            printf("Cmd=%d, NodeId = %d SetVal = %d\r\n", UpperCmd[1], bNodeID, Control_Data);
            ch = '*';
            break;
          case 4:		//Add Node
            bNodeID = UpperCmd[2];
            Control_Data = UpperCmd[3];
            printf("Cmd=%d, Add node\r\n", UpperCmd[1]);
            ch = 'A';
            break;
          case 5:		//Query Stats
            bNodeID = UpperCmd[2];
            printf("Cmd=%d, Query Node[%d] status\r\n", UpperCmd[1], bNodeID);
            break;
          case 6:		//Reset Default
            printf("Cmd=%d, Reset to Default\r\n", UpperCmd[1]);
            ch = 'D';
            break;
          case 7:		//Query Stats
            printf("Cmd=%d, Set OTG\r\n", UpperCmd[1]);
            if (UpperCmd[2] == 0)
            {
              printf("\r\nUsb OTG Disable\r\n");
#if 1
              GPIO_InitTypeDef GPIO_InitStruct;
              
              /*Configure GPIO pin : PB5 - Open Drain*/
              GPIO_InitStruct.Pin = GPIO_PIN_5;
              GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
              
              /*Configure GPIO pin : PB6 */
              GPIO_InitStruct.Pin = GPIO_PIN_6;
              GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState) GPIO_PIN_RESET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState) GPIO_PIN_SET);
            }
            else
            {
              
              printf("\r\nUsb OTG Enable\r\n");
#if 1
              GPIO_InitTypeDef GPIO_InitStruct;
              
              /*Configure GPIO pin : PB5 - Open Drain*/
              GPIO_InitStruct.Pin = GPIO_PIN_5;
              GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
              
              /*Configure GPIO pin : PB6 */
              GPIO_InitStruct.Pin = GPIO_PIN_6;
              GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
              GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
              HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#else
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState) GPIO_PIN_SET);
              HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, (GPIO_PinState) GPIO_PIN_RESET);
#endif
            }
            Control_from = 1;
            response_Upper(0x07, 0x01, 00);
            break; 
          }
          Control_from = 3; 
        }
        else
        {
          ch = 0;
          if ( HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, 1) == HAL_OK)
          {
            HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 1);
          }
          else
          {
            osDelay(10);
            ch = 0;
            continue;
          }
          //            		if (((ch == 'Q') || (ch == 'q')))
          //           			break;     
          Control_from = 0;
        }
        switch (ch)
        {
          
        case 'A': // Add
        case 'a': // Add
          
          fAddNodeToNetwork();
          break;
        case 'R': // Remove
        case 'r': // Remove
          fRemoveNodeFromNetwork();
          break;
          
        case 'F':	/* remove Failed current nodeID */
        case 'f':	/* remove Failed current nodeID */
          fRemoveFailedNode();
          break;
          
        case 'L':	/* Enter LearnMode */
        case 'l':	/* Enter LearnMode */
          fSetLearnMode();
          break;
          
        case '?':
          mainlog_wr("ZW_RequestNetworkUpdate called...\r\n");
          api.ZW_RequestNetworkUpdate(RequestNetworkUpdate_Compl);
          break;
          
        case 'D':	/* call ZW_SetDefault */
        case 'd':	/* call ZW_SetDefault */
          mainlog_wr("ZW_SetDefault called...\r\n");
          api.ZW_SetDefault(SetDefault_Compl);
          break;
          
        case 'W':
        case 'w':
          {
            mainlog_wr("ZW_SoftReset called (watchdog)...\r\n");
            api.ZW_SoftReset(1);
          }
          break;
          
        case 'X':
        case 'x':
          {
            mainlog_wr("ZW_SoftReset called...\r\n");
            api.ZW_SoftReset(0);
          }
          break;
          
        case '.':
          {
            NODEINFO nodeInfo;
            
            api.ZW_GetNodeProtocolInfo(bNodeID, &nodeInfo);
            mainlog_wr("Get Protocol Info called for Node %u : Basic Nodetype = %u\r\n", bNodeID, nodeInfo.nodeType.basic);
          }
          break;
          
        case '*':
          fSendData(Control_from, Control_Data);
          break;
          
        case '/':
          {
            mainlog_wr("ZW_SendNodeInformation Broadcast - start\r\n");
            api.ZW_SendNodeInformation(255, 0, NULL);
            mainlog_wr("ZW_SendNodeInformation Broadcast - done\r\n");
          }
          break;
          
        case 'V': // Get version
        case 'v': // Get version
          fVersion();
          break;
          
        case 'P':
        case 'p':
          fGetCapabilities();
          break;
          
          
        case 'I': // Initialize nodeList with node information from device module
        case 'i': // Initialize nodeList with node information from device module
          fReloadNodeList();
          break;
          
        case 'U':
        case 'u':
          mainlog_wr("ZW_RequestNodeNeighborUpdate\r\n");
          EnterNewCurrentNodeID();
          api.ZW_RequestNodeNeighborUpdate(bNodeID, RequestNodeNeighborUpdate_Compl);
          break;
          
        case '!':
          printf("Toggling incomming notifications ");
          if (printIncomming == true)
          {
            printf("off\r\n");
            printIncomming = false;
          }
          else
          {
            printf("on\r\n");
            printIncomming = true;
          }
          break;
          
        case '#':
          EnterNewCurrentNodeID();
          break;
          
        case '"':
          mainlog_wr("Request NodeInformation for node %u\r\n", bNodeID);
          if (api.ZW_RequestNodeInfo(bNodeID, RequestNodeInfo_Compl))
          {
            mainlog_wr("ZW_RequestNodeInfo initiated for node %u\r\n", bNodeID);
          }
          else
          {
            mainlog_wr("ZW_RequestNodeInfo could not initiated for node %u\r\n", bNodeID);
          }
          break;
          
        case '%':
          DumpNodeInfo(bNodeID);
          break;
          
        case 'H': // Help
        case 'h': // Help
          printf("\n");
          printf("A   : Add Node (only supported by Controller)\r\n");
          printf("R   : Remove Node (only supported by Controller)\r\n");
          printf("F   : remove Failed current nodeID %03d\r\n", bNodeID);
          printf("U   : ZW_RequestNodeNeighborUpdate\r\n");
          printf("#   : Set current nodeID\r\n");
          printf(".   : Call ZW_GetNodeProtocolInfo for current nodeID\r\n");
          printf("\"   : Call ZW_RequestNodeInfo for current nodeID %03d\r\n", bNodeID);
          printf("*   : ZW_SendData with explore to current NodeID (%03d)\r\n", bNodeID);
          printf("%%   : Dump NodeInfo for current NodeID %03d\r\n", bNodeID);
          printf("/   : Broadcast Node Information frame\r\n");
          printf("P   : Get serialapi capabilities\r\n");
          printf("V   : get Version\r\n");
          printf("I   : Initialize nodeList and dump nodeinfo (only supported by Controller)\r\n");
          printf("L   : ZW_SetLearnMode\r\n");
          printf("D   : ZW_SetDefault\r\n");
          printf("W   : ZW_SoftReset\r\n");
          printf("!   : Print incomming frames - %s\r\n", printIncomming ? "TRUE" : "FALSE");
          //                        printf("Q   : Quit\r\n");
          break;
          
        default :
          osDelay(10);
          break;
        }
      }
    }
    api.Shutdown();
    osTimerStop(ZWhostAppTimerHandle);
    printf("ZWhostAppTimer stop \r\n");
    printf("Failed to get libType so Stop operation \r\n");
    while(1)
    {
      if ( HAL_UART_Receive(&huart2, (uint8_t *)&ch, 1, 1) == HAL_OK)
      {
        HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 1);
        printf("The communication with ZW Module was failed so that reboot target for operation ...\r\n");
      }
      else
      {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_7);
        osDelay(1000);
      }
    }
    
  }
  else
  {
    mainlog_wr("No port detected on serial port %s\r\n", portstr);
  }
}

#define	DEL			0x7F

int
UARTgetsNonblocking(char *pcBuf, uint32_t ui32Len)
{
  uint32_t ui32Count = 0;
  int8_t cChar;
  static int8_t bLastWasCR = 0;
  
  //
  // Adjust the length back by 1 to leave space for the trailing
  // null terminator.
  //
  ui32Len--;
  
  //
  // Process characters until a newline is received.
  //
  while(1)
  {
    //
    // Read the next character from the console.
    //
    while (HAL_UART_Receive(&huart2, (uint8_t*) &cChar, 1, 1) != HAL_OK )
    {
      osDelay(100);
    }
    
    //
    // See if the backspace key was pressed.
    //
    
    if((cChar == '\b' )|| (cChar == DEL ))	//JY Koh 0x7F
    {
      //
      // If there are any characters already in the buffer, then delete
      // the last.
      //
      if(ui32Count)
      {
        //
        // Rub out the previous character.
        //;
        HAL_UART_Transmit(&huart2, (uint8_t *)"\b \b", 3, 0xFFFF);
        //
        // Decrement the number of characters in the buffer.
        //
        ui32Count--;
      }
      
      //
      // Skip ahead to read the next character.
      //
      continue;
    }
    
    //
    // If this character is LF and last was CR, then just gobble up the
    // character because the EOL processing was taken care of with the CR.
    //
    if((cChar == '\n') && bLastWasCR)
    {
      bLastWasCR = 0;
      continue;
    }
    
    //
    // See if a newline or escape character was received.
    //
    if((cChar == '\r') || (cChar == '\n') || (cChar == 0x1b))
    {
      //
      // If the character is a CR, then it may be followed by a LF which
      // should be paired with the CR.  So remember that a CR was
      // received.
      //
      if(cChar == '\r')
      {
        bLastWasCR = 1;
      }
      
      //
      // Stop processing the input and end the line.
      //
      break;
    }
    
    //
    // Process the received character as long as we are not at the end of
    // the buffer.  If the end of the buffer has been reached then all
    // additional characters are ignored until a newline is received.
    //
    if(ui32Count < ui32Len)
    {
      //
      // Store the character in the caller supplied buffer.
      //
      pcBuf[ui32Count] = cChar;
      
      //
      // Increment the count of characters received.
      //
      ui32Count++;
      
      //
      // Reflect the character back to the user.
      //
      //            MAP_UARTCharPut(g_ui32Base, cChar);
      HAL_UART_Transmit(&huart2, (uint8_t *)&cChar, 1, 0xFFFF);
    }
  }
  
  //
  // Add a null termination to the string.
  //
  pcBuf[ui32Count] = 0;
  
  //
  // Send a CRLF pair to the terminal to end the line.
  //
  HAL_UART_Transmit(&huart2, (uint8_t *)"\r\n", 2, 0xFFFF);
  //
  // Return the count of int8_ts in the buffer, not counting the trailing 0.
  //
  return(ui32Count);
  
}
