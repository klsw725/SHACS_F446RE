/****************************************************************************
 *
 * Z-Wave, the wireless language.
 *
 * Copyright (c) 2001-2011
 * Sigma Designs, Inc.
 * All Rights Reserved
 *
 *---------------------------------------------------------------------------
 *
 * Description: Headerfile for the IMAtoolbox Z-Wave Network Health Test
 *
 * Last Changed By:  $Author: $ 
 * Revision:         $Rev: $
 * Last Changed:     $Date: $
 * History
   -  2017.11.06 Modified by JY Koh for SHACS project
 ****************************************************************************/

#if !defined(_ZPhostApp_H_)
#define _ZPhostApp_H_

static BYTE bMaxNodeID;
static BYTE nodeState[ZW_MAX_NODES];
static BYTE nodeTypeList[] = 
	{0,									//No device
		GENERIC_TYPE_GENERIC_CONTROLLER,
		GENERIC_TYPE_STATIC_CONTROLLER, 
		GENERIC_TYPE_SWITCH_BINARY,
		GENERIC_TYPE_SWITCH_MULTILEVEL,
		GENERIC_TYPE_SENSOR_BINARY,
		GENERIC_TYPE_SENSOR_MULTILEVEL,
		GENERIC_TYPE_METER_PULSE,
		GENERIC_TYPE_ENTRY_CONTROL,
		0xFF							//Unkown device
	};


/* SerialAPIGetCapabilities structure definition */
typedef struct _S_SERIALAPI_CAPABILITIES
{
	BYTE bSerialAPIApplicationVersion;
	BYTE bSerialAPIApplicationRevision;
	BYTE bSerialAPIManufacturerID1;
	BYTE bSerialAPIManufacturerID2;
	BYTE bSerialAPIManufacturerProductType1;
	BYTE bSerialAPIManufacturerProductType2;
	BYTE bSerialAPIManufacturerProductID1;
	BYTE bSerialAPIManufacturerProductID2;
	BYTE abFuncIDSupportedBitmask[256 / 8];
} S_SERIALAPI_CAPABILITIES;

/* LWR definitions - Only valid for 4.5x static controllers */
#define LWR_OFFSET_4_54_02_STATIC_NOSUC_NOREP	(WORD)(0x10000 - 0x2C00 + 0x268E)
#define LWR_ENTRY_SIZE							5
#define LWR_ENTRY_OFFSET_REPEATER_1				0
#define LWR_ENTRY_OFFSET_REPEATER_2				1
#define LWR_ENTRY_OFFSET_REPEATER_3				2
#define LWR_ENTRY_OFFSET_REPEATER_4				3
#define LWR_ENTRY_OFFSET_CONF					4

#define LWR_DIRECT_ROUTE						0xFE


#ifdef __cplusplus
extern "C" {
#endif


void ZWApp_main(void const * argument);


void get_NodeInfomation(BYTE *get_nodeList, BYTE *get_nodeType, BYTE *NumofNode);

#ifdef __cplusplus
}
#endif
#endif // !defined(_ZPhostApp_H_)
