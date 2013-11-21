/*
      SafeRTOS Copyright (C) Wittenstein High Integrity Systems.

      See projdefs.h for version number information.

      SafeRTOS has been licensed by Wittenstein High Integrity Systems to
      Texas Instruments to be embedded within the ROM of certain processors.
      If SafeRTOS is embedded in the ROM of your TI processor you may copy and
      use this header file to facilitate the use of SafeRTOS as described in the
      User Manual.

      If you use SafeRTOS in a safety related application or it is required to meet
      high dependability requirements you must use SafeRTOS in accordance with the
      Safety Manual which forms a part of the Design Assurance Pack (DAP). The
      DAP and support may be purchased separately from WITTENSTEIN high integrity
      systems.

      WITTENSTEIN high integrity systems is a trading name of WITTENSTEIN
      aerospace & simulation ltd, Registered Office: Brown's Court, Long Ashton
      Business Park, Yanley Lane, Long Ashton, Bristol, BS41 9LB, UK.
      Tel: +44 (0) 1275 395 600, fax: +44 (0) 1275 393 630.
      E-mail: info@HighIntegritySystems.com
      Registered in England No. 3711047; VAT No. GB 729 1583 15

      http://www.SafeRTOS.com
*/

#ifndef PROJDEFS_H
#define PROJDEFS_H

typedef void (*pdTASK_CODE)( void *pvParameters );

#define pdTRUE		( ( portBASE_TYPE ) 1 )
#define pdFALSE		( ( portBASE_TYPE ) 0 )

#define pdPASS		( ( portBASE_TYPE ) 1 )
#define pdFAIL		( ( portBASE_TYPE ) 0 )

/* Error definitions. */
#define errSUPPLIED_BUFFER_TOO_SMALL			( ( portBASE_TYPE ) -1 )
#define errINVALID_PRIORITY						( ( portBASE_TYPE ) -2 )
/* -3 has been removed. */
#define errQUEUE_FULL							( ( portBASE_TYPE ) -4 )
#define errINVALID_BYTE_ALIGNMENT				( ( portBASE_TYPE ) -5 )
#define errNULL_PARAMETER_SUPPLIED				( ( portBASE_TYPE ) -6 )
#define errINVALID_QUEUE_LENGTH					( ( portBASE_TYPE ) -7 )
#define errINVALID_TASK_CODE_POINTER			( ( portBASE_TYPE ) -8 )
#define errSCHEDULER_IS_SUSPENDED               ( ( portBASE_TYPE ) -9 )
#define errINVALID_TASK_HANDLE                  ( ( portBASE_TYPE ) -10 )
#define errDID_NOT_YIELD                        ( ( portBASE_TYPE ) -11 )
#define errTASK_ALREADY_SUSPENDED               ( ( portBASE_TYPE ) -12 )
#define errTASK_WAS_NOT_SUSPENDED               ( ( portBASE_TYPE ) -13 )
#define errNO_TASKS_CREATED                     ( ( portBASE_TYPE ) -14 )
#define errSCHEDULER_ALREADY_RUNNING            ( ( portBASE_TYPE ) -15 )
#define errINVALID_QUEUE_HANDLE                 ( ( portBASE_TYPE ) -17 )
#define errERRONEOUS_UNBLOCK                    ( ( portBASE_TYPE ) -18 )
#define errQUEUE_EMPTY                          ( ( portBASE_TYPE ) -19 )
#define errINVALID_TICK_VALUE					( ( portBASE_TYPE ) -20 )
#define errINVALID_TASK_SELECTED				( ( portBASE_TYPE ) -21 )
#define errTASK_STACK_OVERFLOW					( ( portBASE_TYPE ) -22 )
#define errSCHEDULER_WAS_NOT_SUSPENDED 			( ( portBASE_TYPE ) -23 )
#define errINVALID_BUFFER_SIZE					( ( portBASE_TYPE ) -24 )
#define errBAD_OR_NO_TICK_RATE_CONFIGURATION	( ( portBASE_TYPE ) -25 )
#define errBAD_HOOK_FUNCTION_ADDRESS			( ( portBASE_TYPE ) -26 )
#define errERROR_IN_VECTOR_TABLE				( ( portBASE_TYPE ) -27 )

#define errINVALID_portQUEUE_OVERHEAD_BYTES_SETTING ( ( portBASE_TYPE ) -1000 )
#define errINVALID_SIZEOF_TCB						( ( portBASE_TYPE ) -1001 )
#define errINVALID_SIZEOF_QUEUE_STRUCTURE			( ( portBASE_TYPE ) -1002 )

#endif /* PROJDEFS_H */


