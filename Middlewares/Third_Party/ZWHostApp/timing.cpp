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
 * Description: Timing module
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

//#include <sys/timeb.h>
#include "timing.h"
//#include <time.h>
#include "cmsis_os.h"

LARGE_INTEGER timeFreq;


/*=============================== TimingStart ================================
** Function description
**      Sample High Performance timer for Start time in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER
TimingStart(
    sTiming *timer)
{

    return timer->startTime;
}


/*=============================== TimingStop =================================
** Function description
**      Sample High Performance timer Stop time and calculate Elapsed time
**		in High Performance Ticks
** Side effects:
**		
**--------------------------------------------------------------------------*/
LARGE_INTEGER
TimingStop(
    sTiming *timer)
{

    return timer->stopTime;
}


/*=========================== TimingGetElapsedMSec ===========================
** Function description
**      Returns the milli seconds which elapsedTime represent. 
**		ElapsedTime is the number of High Performance Ticks which for a given
**      period has past.
**      if 0 then either 0 milliseconds has past or milliseconds cannot be
**		returned due to the High Performance Counter Resolution.
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
double
TimingGetElapsedMSec(
    LARGE_INTEGER elapsedTime)
{
    double elapsedMSec;
    /* Get the resolution of the High Performance timer in ticks pr. second */
    return elapsedMSec;
}


/*========================= TimingGetElapsedMSecDbl ==========================
** Function description
**      Returns the milli seconds which elapsedTime represent. 
**		ElapsedTime is the number of High Performance Ticks which for a given
**      period has past.
**      if 0 then either 0 milliseconds has past or milliseconds cannot be
**		returned due to the High Performance Counter Resolution.
**
** Side effects:
**		
**--------------------------------------------------------------------------*/
double
TimingGetElapsedMSecDbl(
    double elapsedTime)
{
    double elapsedMSec;

    return elapsedMSec;
}

//**********************************************************************
//                    CTimerJob
//**********************************************************************

/*=============================== GetTimeOutSec =============================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
time_t  CTimerJob::GetTimeOutSec()
{
    return timeOutSec;
}


/*=============================== Callback =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTimerJob::Callback()
{

}

/*=============================== GetHandle =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
int CTimerJob::GetHandle()
{
    return handle;
}

//**********************************************************************
//                    CTime
//**********************************************************************

CTime* CTime::pTimeSingleton = NULL;

/*=============================== CTime =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime::CTime()
{

}

/*=============================== ~CTime =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime::~CTime()
{
}

/*=============================== GetInstance =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
CTime* CTime::GetInstance()
{

}

/*=============================== Timeout =================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
int CTime::Timeout( time_t _time, void (*CbTimeOut)(int H))
{

    int h;

    return h;
}
    
/*=============================== Kill ======================================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTime::Kill( int h)
{

}

/*=============================== TimerEngine ===============================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
void CTime::TimerEngine()
{
	
}
/*=============================== TimeOutThreadFunc ==========================
** Function description
** Side effects:
**--------------------------------------------------------------------------*/
unsigned __stdcall CTime::TimeOutThreadFunc( void* dummy )
{


    return 0;
} 

/*=============================== GetHandle =================================
** Function description
** Find a free handle.
**--------------------------------------------------------------------------*/
int CTime::GetHandle()
{
    int h = 1;

    return h;
}


/*=============================== GetToken  ================================
** Function description
** Get token to start working on array. Current thread is pause if another 
** thread is working on array.
**--------------------------------------------------------------------------*/
void CTime::GetToken()
{

}

/*=============================== GetToken  ================================
** Function description
** Free token must be called after finish working on array! Otherwise, the array is 
** locked forever.
**--------------------------------------------------------------------------*/
void CTime::FreeToken()
{

}

