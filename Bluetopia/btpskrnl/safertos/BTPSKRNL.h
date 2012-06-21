/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  BTPSKRNL.h - Stellaris(R) Bluetooth Stack OS Kernel Abstraction Type      */
/*               Definitions, Constants and Prototypes for Stonestreet One,   */
/*               LLC Bluetopia(R) Bluetooth Protocol Stack.                   */
/*                                                                            */
/******************************************************************************/
#ifndef __BTPSKRNLH__
#define __BTPSKRNLH__

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "BTTypes.h"            /* Bluetooth basic type definitions           */

  /* If building with a C++ compiler, make all of the definitions in    */
  /* this header have a C binding.                                      */
#ifdef __cplusplus

extern "C"
{

#endif

   /* Texas Instruments Stellaris specific Implementation Extensions.   */

   /* The following declared type represents the Prototype Function for */
   /* a function that can be registered with the BTPSKRNL module to     */
   /* receive output characters.  This function will be called whenever */
   /* BTPS_OutputMessage() or BTPS_DumpData() is called (or if debug is */
   /* is enabled - DEBUG preprocessor symbol, whenever the DBG_MSG() or */
   /* DBG_DUMP() MACRO is used and there is debug data to output.       */
   /* * NOTE * This function can be registered by passing the address   */
   /*          of the implementation function in the                    */
   /*          MessageOutputCallback member of the                      */
   /*          BTPS_Initialization_t structure which is passed to the   */
   /*          BTPS_Init() function.  If no function is registered then */
   /*          there will be no output (i.e. it will simply be ignored).*/
typedef void (BTPSAPI *BTPS_MessageOutputCallback_t)(char DebugCharacter);

   /* The following structure represents the structure that is passed   */
   /* to the BTPS_Init() function to notify the Bluetooth abstraction   */
   /* layer of the function(s) that are required for proper device      */
   /* functionality.                                                    */
typedef struct _tagBTPS_Initialization_t
{
   BTPS_MessageOutputCallback_t MessageOutputCallback;
} BTPS_Initialization_t;

#define BTPS_INITIALIZATION_SIZE                         (sizeof(BTPS_Initialization_t))

#include "BKRNLAPI.h"           /* Bluetooth Kernel Prototypes/Constants.     */

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State from an Interrupt.     */
   /* Once the Event is in this State, ALL calls to the BTPS_WaitEvent()*/
   /* function will return.  This function accepts as input the Event   */
   /* Handle of the Event to set to the Signalled State.                */
BTPSAPI_DECLARATION void BTPSAPI BTPS_INT_SetEvent(Event_t Event);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_INT_SetEvent_t)(Event_t Event);
#endif

   /* The following function is the Application Idle Hook that should be*/
   /* called during the Idle Task.  This function is responsible for    */
   /* processing any currently queued output information (anything that */
   /* has been queued for writing via the BTPS_OutputMessage() or the   */
   /* BTPS_DumpData() functions.                                        */
BTPSAPI_DECLARATION void BTPSAPI BTPS_ApplicationIdleHook(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_ApplicationIdleHook_t)(void);
#endif

   /* Mark the end of the C bindings section for C++ compilers.         */
#ifdef __cplusplus

}

#endif

#endif
