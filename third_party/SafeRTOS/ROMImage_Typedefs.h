/*
	SafeRTOS Copyright (C) Wittenstein High Integrity Systems.

	See projdefs.h for version number information.

	SafeRTOS is distributed exclusively by Wittenstein High Integrity Systems,
	and is subject to the terms of the License granted to your organization,
	including its warranties and limitations on distribution.  It cannot be
	copied or reproduced in any way except as permitted by the License.

	Licenses are issued for each concurrent user working on a specified product
	line.

	WITTENSTEIN high integrity systems is a trading name of WITTENSTEIN
	aerospace & simulation ltd, Registered Office: Brown's Court,, Long Ashton
	Business Park, Yanley Lane, Long Ashton, Bristol, BS41 9LB, UK.
	Tel: +44 (0) 1275 395 600, fax: +44 (0) 1275 393 630.
	E-mail: info@HighIntegritySystems.com
	Registered in England No. 3711047; VAT No. GB 729 1583 15

	http://www.SafeRTOS.com
*/

#ifndef ROM_IMAGE_TYPEDEFS
#define ROM_IMAGE_TYPEDEFS

#include "SafeRTOS/SafeRTOS.h"
#include "SafeRTOS/queue.h"
#include "SafeRTOS/task.h"

typedef void				( *type_vSafeRTOSPendSVHandler )( void );
typedef void				( *type_vSafeRTOSSVCHandler )( void );
typedef void				( *type_vPortSysTickHandler )( void );
typedef void				( *type_vPortEnterCritical )( void );
typedef void				( *type_vPortExitCritical )( void );
typedef void				( *type_vPortYield )( void );
typedef unsigned portLONG	( *type_ulPortSetInterruptMaskFromISR )( void );
typedef void				( *type_vPortClearInterruptMaskFromISR )( unsigned portLONG ulOriginalMask );
typedef void 				( *type_vTaskInitializeScheduler )( signed portCHAR *pcInIdleTaskStackBuffer, unsigned portLONG ulInIdleTaskStackSizeBytes, unsigned portLONG ulInAdditionalStackCheckMarginBytes, const xPORT_INIT_PARAMETERS * const pxPortInitParameters );
typedef portBASE_TYPE		( *type_xTaskCreate )( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, signed portCHAR *pcStackBuffer, unsigned portLONG ulStackDepthBytes, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pvCreatedTask );
typedef portBASE_TYPE		( *type_xTaskDelete )( xTaskHandle pxTask );
typedef portBASE_TYPE		( *type_xTaskDelay )( portTickType xTicksToDelay );
typedef portBASE_TYPE		( *type_xTaskDelayUntil )( portTickType *pxPreviousWakeTime, portTickType xTimeIncrement );
typedef portBASE_TYPE		( *type_xTaskPriorityGet )( xTaskHandle pxTask, unsigned portBASE_TYPE *puxPriority );
typedef portBASE_TYPE		( *type_xTaskPrioritySet )( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority );
typedef portBASE_TYPE		( *type_xTaskSuspend )( xTaskHandle pxTaskToSuspend );
typedef portBASE_TYPE		( *type_xTaskResume )( xTaskHandle pxTaskToResume );
typedef portBASE_TYPE		( *type_xTaskStartScheduler )( portBASE_TYPE xUseKernelConfigurationChecks );
typedef void				( *type_vTaskSuspendScheduler )( void );
typedef portBASE_TYPE		( *type_xTaskResumeScheduler )( void );
typedef portTickType		( *type_xTaskGetTickCount )( void );
typedef portBASE_TYPE		( *type_xQueueCreate )( signed portCHAR *pcQueueMemory, unsigned portBASE_TYPE uxBufferLength, unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize, xQueueHandle *pxQueue );
typedef portBASE_TYPE		( *type_xQueueSend )( xQueueHandle pxQueue, const void * pvItemToQueue, portTickType xTicksToWait );
typedef portBASE_TYPE		( *type_xQueueReceive )( xQueueHandle pxQueue, void *pvBuffer, portTickType xTicksToWait );
typedef portBASE_TYPE		( *type_xQueueMessagesWaiting )( const xQueueHandle pxQueue, unsigned portBASE_TYPE *puxMessagesWaiting );
typedef portBASE_TYPE		( *type_xQueueSendFromISR )( xQueueHandle pxQueue, const void *pvItemToQueue, portBASE_TYPE *pxHigherPriorityTaskWoken );
typedef portBASE_TYPE		( *type_xQueueReceiveFromISR )( xQueueHandle pxQueue, void *pvBuffer, portBASE_TYPE *pxTaskWoken );
typedef portBASE_TYPE		( *type_xQueueIsQueueEmptyFromISR )( const xQueueHandle pxQueue );
typedef portBASE_TYPE		( *type_xQueueIsQueueFullFromISR )( const xQueueHandle xQueue );
typedef portBASE_TYPE		( *type_xQueueMessagesWaitingFromISR )( const xQueueHandle pxQueue, unsigned portBASE_TYPE *puxMessagesWaiting );

typedef struct SAFE_RTOS_ROM_IMAGE
{
	signed portCHAR *pcVersionNumber;

	type_vSafeRTOSSVCHandler rom_vSafeRTOSSVCHandler;
	type_vPortSysTickHandler rom_vPortSysTickHandler;
	type_vSafeRTOSPendSVHandler rom_vSafeRTOSPendSVHandler;

	type_vPortEnterCritical rom_vPortEnterCritical;
	type_vPortExitCritical rom_vPortExitCritical;
	type_vPortYield rom_vPortYield;
	type_ulPortSetInterruptMaskFromISR rom_ulPortSetInterruptMaskFromISR;
	type_vPortClearInterruptMaskFromISR rom_vPortClearInterruptMaskFromISR;
	type_vTaskInitializeScheduler rom_vTaskInitializeScheduler;
	type_xTaskCreate rom_xTaskCreate;
	type_xTaskDelete rom_xTaskDelete;
	type_xTaskDelay rom_xTaskDelay;
	type_xTaskDelayUntil rom_xTaskDelayUntil;
	type_xTaskPriorityGet rom_xTaskPriorityGet;
	type_xTaskPrioritySet rom_xTaskPrioritySet;
	type_xTaskSuspend rom_xTaskSuspend;
	type_xTaskResume rom_xTaskResume;
	type_xTaskStartScheduler rom_xTaskStartScheduler;
	type_vTaskSuspendScheduler rom_vTaskSuspendScheduler;
	type_xTaskResumeScheduler rom_xTaskResumeScheduler;
	type_xTaskGetTickCount rom_xTaskGetTickCount;
	type_xQueueCreate rom_xQueueCreate;
	type_xQueueSend rom_xQueueSend;
	type_xQueueReceive rom_xQueueReceive;
	type_xQueueMessagesWaiting rom_xQueueMessagesWaiting;
	type_xQueueSendFromISR rom_xQueueSendFromISR;
	type_xQueueReceiveFromISR rom_xQueueReceiveFromISR;
	type_xQueueIsQueueEmptyFromISR rom_xQueueIsQueueEmptyFromISR;
	type_xQueueIsQueueFullFromISR rom_xQueueIsQueueFullFromISR;
	type_xQueueMessagesWaitingFromISR rom_xQueueMessagesWaitingFromISR;

	void *pvReserved1;
	void *pvReserved2;
	void *pvReserved3;
	void *pvReserved4;
	void *pvReserved5;
	void *pvReserved6;
	void *pvReserved7;
	void *pvReserved8;
	void *pvReserved9;
	void *pvReserved10;
	
} xSAFE_RTOS_ROM_IMAGE;

#endif



