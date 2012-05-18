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
/*
	API documentation is included in the user manual.  Do not refer to this
	file for documentation.
*/

#ifndef TASK_H
#define TASK_H

#include "SafeRTOS/list.h"

/*-----------------------------------------------------------
 * MACROS AND DEFINITIONS
 *----------------------------------------------------------*/

typedef void *xTaskHandle;

typedef struct xTIME_OUT
{
    unsigned portBASE_TYPE uxOverflowCount;
    portTickType  xTimeOnEntering;
} xTimeOutType;

#define tskIDLE_PRIORITY						( ( unsigned portBASE_TYPE ) 0 )
#define taskYIELD()								portYIELD()
#define taskYIELD_FROM_ISR( xSwitchRequired )	portYIELD_FROM_ISR( xSwitchRequired )
#define taskENTER_CRITICAL()					portENTER_CRITICAL()
#define taskEXIT_CRITICAL()						portEXIT_CRITICAL()
#define taskDISABLE_INTERRUPTS()				portDISABLE_INTERRUPTS()
#define taskENABLE_INTERRUPTS()					portENABLE_INTERRUPTS()
#define taskSET_INTERRUPT_MASK_FROM_ISR()		portSET_INTERRUPT_MASK_FROM_ISR()
#define taskCLEAR_INTERRUPT_MASK_FROM_ISR( uxOriginalPriority )		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxOriginalPriority )


/*-----------------------------------------------------------
 * Public API
 *----------------------------------------------------------*/

portBASE_TYPE xTaskCreate( pdTASK_CODE pvTaskCode, const signed portCHAR * const pcName, signed portCHAR * const pcStackBuffer, unsigned portLONG ulStackDepthBytes, void *pvParameters, unsigned portBASE_TYPE uxPriority, xTaskHandle *pvCreatedTask );
portBASE_TYPE xTaskDelete( xTaskHandle pxTaskToDelete );
portBASE_TYPE xTaskDelay( portTickType xTicksToDelay );
portBASE_TYPE xTaskDelayUntil( portTickType *pxPreviousWakeTime, portTickType xTimeIncrement );
portBASE_TYPE xTaskPriorityGet( xTaskHandle pxTask, unsigned portBASE_TYPE *puxPriority );
portBASE_TYPE xTaskPrioritySet( xTaskHandle pxTask, unsigned portBASE_TYPE uxNewPriority );
portBASE_TYPE xTaskSuspend( xTaskHandle pxTaskToSuspend );
portBASE_TYPE xTaskResume( xTaskHandle pxTaskToResume );
portBASE_TYPE xTaskStartScheduler( portBASE_TYPE xUseKernelConfigurationChecks );
void vTaskSuspendScheduler( void );
portBASE_TYPE xTaskResumeScheduler( void );
portTickType xTaskGetTickCount( void );
void vTaskInitializeScheduler(	signed portCHAR *pcInIdleTaskStackBuffer, unsigned portLONG ulInIdleTaskStackSizeBytes, unsigned portLONG ulInAdditionalStackCheckMarginBytes, const xPORT_INIT_PARAMETERS * const pxPortInitParameters );


/*-----------------------------------------------------------
 * Functions for internal use only.  Not to be called directly from a host
 * application or task.
 *----------------------------------------------------------*/

void vTaskIncrementTick( void );
void vTaskPlaceOnEventList( xList *pxEventList, portTickType xTicksToWait );
portBASE_TYPE xTaskRemoveFromEventList( const xList *pxEventList );
void vTaskSelectNextTask( void );
xTaskHandle xTaskGetCurrentTaskHandle( void );
portBASE_TYPE xTaskIsSchedulerSuspended( void );
void vTaskStackCheckFailed( void );
void vTaskSetTimeOut( xTimeOutType *pxTimeOut );
portBASE_TYPE xTaskCheckForTimeOut( xTimeOutType *pxTimeOut, portTickType *pxTicksToWait );
void vTaskPendYield( void );

#endif /* TASK_H */



