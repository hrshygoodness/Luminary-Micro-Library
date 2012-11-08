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
#ifndef SAFERTOS_API_H
#define SAFERTOS_API_H

#include "SafeRTOS/ROMImage_Typedefs.h"


/* Define handler addresses for use in vector table */
#define vSafeRTOS_PendSV_Handler_Address	( 0x02000fdd )
#define vSafeRTOS_SVC_Handler_Address		( 0x02000f45 )
#define vSafeRTOS_SysTick_Handler_Address	( 0x02001175 )


/* Define jump table entries for SafeRTOS kernel functions */
#define SAFE_RTOS_JUMP_TABLE_START			( 0x02000000 )

#define vSafeRTOSPendSVHandler( )										( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vSafeRTOSPendSVHandler( ) )
#define vSafeRTOSSVCHandler( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vSafeRTOSSVCHandler( ) )
#define vPortSysTickHandler( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vPortSysTickHandler( ) )
#define vPortEnterCritical( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vPortEnterCritical( ) )
#define vPortExitCritical( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vPortExitCritical( ) )
#define vPortYield( )													( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vPortYield( ) )
#define ulPortSetInterruptMaskFromISR( )								( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_ulPortSetInterruptMaskFromISR( ) )
#define vPortClearInterruptMaskFromISR( ulOriginalMask )				( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vPortClearInterruptMaskFromISR( ( ulOriginalMask ) ) )
#define vTaskInitializeScheduler( pcInIdleTaskStackBuffer,  ulInIdleTaskStackSizeBytes, ulInAdditionalStackCheckMarginBytes, pxPortInitParameters )	( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vTaskInitializeScheduler( ( pcInIdleTaskStackBuffer ), ( ulInIdleTaskStackSizeBytes ), ( ulInAdditionalStackCheckMarginBytes ), ( pxPortInitParameters ) ) )
#define xTaskCreate( pvTaskCode, pcName, pcStackBuffer, ulStackDepthBytes, pvParameters, uxPriority, pvCreatedTask ) ( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskCreate(  ( pvTaskCode ), ( pcName ), ( pcStackBuffer ), ( ulStackDepthBytes ), ( pvParameters ), ( uxPriority ), ( pvCreatedTask ) ) )
#define xTaskDelete( pxTask )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskDelete( ( pxTask ) ) )
#define xTaskDelay( xTicksToDelay )										( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskDelay( ( xTicksToDelay ) ) )
#define xTaskDelayUntil( pxPreviousWakeTime, xTimeIncrement )			( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskDelayUntil( ( pxPreviousWakeTime ),  ( xTimeIncrement ) ) )
#define xTaskPriorityGet( pxTask, puxPriority )							( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskPriorityGet( ( pxTask ), ( puxPriority ) ) )
#define xTaskPrioritySet( pxTask, uxNewPriority )						( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskPrioritySet( ( pxTask ), ( uxNewPriority ) ) )
#define xTaskSuspend( pxTaskToSuspend )									( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskSuspend( (pxTaskToSuspend ) ) )
#define xTaskResume( pxTaskToResume )									( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskResume( (pxTaskToResume ) ) )
#define xTaskStartScheduler( xUseKernelConfigurationChecks )			( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskStartScheduler( ( xUseKernelConfigurationChecks ) ) )
#define vTaskSuspendScheduler( )										( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_vTaskSuspendScheduler( ) )
#define xTaskResumeScheduler( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskResumeScheduler( ) )
#define xTaskGetTickCount( )											( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xTaskGetTickCount( ) )
#define xQueueCreate( pcQueueMemory, uxBufferLength, uxQueueLength, uxItemSize, pxQueue ) ( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueCreate( ( pcQueueMemory ), ( uxBufferLength ), ( uxQueueLength ), ( uxItemSize ), ( pxQueue ) ) )
#define xQueueSend( pxQueue, pvItemToQueue, xTicksToWait )				( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueSend( ( pxQueue ), ( pvItemToQueue ), ( xTicksToWait ) ) )
#define xQueueReceive( pxQueue, pvBuffer, xTicksToWait )				( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueReceive(  (pxQueue),  ( pvBuffer), ( xTicksToWait ) ) )
#define xQueueMessagesWaiting( pxQueue, puxMessagesWaiting )			( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueMessagesWaiting( ( pxQueue), ( puxMessagesWaiting) ) )
#define xQueueSendFromISR( pxQueue, pvItemToQueue, pxTaskPreviouslyWoken ) ( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueSendFromISR( ( pxQueue), ( pvItemToQueue),  ( pxTaskPreviouslyWoken) ) )
#define xQueueReceiveFromISR( pxQueue, pvBuffer, pxTaskWoken )			( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueReceiveFromISR( ( pxQueue), ( pvBuffer ), (pxTaskWoken) ) )
#define xQueueIsQueueEmptyFromISR( pxQueue )							( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueIsQueueEmptyFromISR( ( pxQueue ) ) )
#define xQueueIsQueueFullFromISR( xQueue )								( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueIsQueueFullFromISR( ( xQueue ) ) )
#define xQueueMessagesWaitingFromISR( pxQueue, puxMessagesWaiting )		( ( ( xSAFE_RTOS_ROM_IMAGE * ) SAFE_RTOS_JUMP_TABLE_START )->rom_xQueueMessagesWaitingFromISR( ( pxQueue ), ( puxMessagesWaiting ) ) )

#endif

