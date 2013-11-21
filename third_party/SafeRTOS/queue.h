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

