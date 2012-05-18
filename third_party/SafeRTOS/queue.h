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

#ifndef QUEUE_H
#define QUEUE_H

typedef void * xQueueHandle; /*lint !e452 this is declared void * outside of the queue.c file for data hiding purposes. */

/*lint -e18 xQueueHandle is declared differently here from within queue.c for data hiding purposes. */

portBASE_TYPE xQueueCreate( signed portCHAR *pcQueueMemory, unsigned portBASE_TYPE uxBufferLength, unsigned portBASE_TYPE uxQueueLength, unsigned portBASE_TYPE uxItemSize, xQueueHandle *pxQueue );
portBASE_TYPE xQueueSend( xQueueHandle pxQueue, const void * const pvItemToQueue, portTickType xTicksToWait );
portBASE_TYPE xQueueReceive( xQueueHandle pxQueue, void * const pvBuffer, portTickType xTicksToWait );
portBASE_TYPE xQueueMessagesWaiting( const xQueueHandle pxQueue, unsigned portBASE_TYPE *puxMessagesWaiting );
portBASE_TYPE xQueueSendFromISR( xQueueHandle pxQueue, const void * const pvItemToQueue, portBASE_TYPE *pxHigherPriorityTaskWoken );
portBASE_TYPE xQueueReceiveFromISR( xQueueHandle pxQueue, void * const pvBuffer, portBASE_TYPE *pxHigherPriorityTaskWoken );
portBASE_TYPE xQueueIsQueueEmptyFromISR( const xQueueHandle pxQueue );
portBASE_TYPE xQueueIsQueueFullFromISR( const xQueueHandle pxQueue );
portBASE_TYPE xQueueMessagesWaitingFromISR( const xQueueHandle pxQueue, unsigned portBASE_TYPE *puxMessagesWaiting );

/*lint +e18 xQueueHandle is declared differently here from within queue.c for data hiding purposes. */

#endif /* QUEUE_H */

