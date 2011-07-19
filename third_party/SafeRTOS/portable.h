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

/*-----------------------------------------------------------
 * Portable layer API.  Each function must be defined for each port.
 *----------------------------------------------------------*/

#ifndef PORTABLE_H
#define PORTABLE_H

#include <SafeRTOS/portmacro.h>

/* Provided as a convenient method of ensuring byte buffers are always aligned
correctly for word access.  This definition can be used to allocate buffers that
are used by queues. */
#define portALIGNED_BUFFER_DECL( size )	union { unsigned long ulAlignmentVar; signed char cBuffer[ size ]; }

/*
 * Setup the stack of a new task so it is ready to be placed under the
 * scheduler control.  The registers have to be placed on the stack in
 * the order that the port expects to find them.
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters );

/*
 * Setup the hardware ready for the scheduler to take control.  This generally
 * sets up a tick interrupt and sets timers for the correct tick frequency.
 *
 * The scheduler will only be started if a set of pre-conditions are met.
 * These pre-conditions can be ignored by setting to 
 * xUseKernelConfigurationChecks to pdFALSE - in which case the scheduler will
 * be started regardless of its state.  This can be useful in ROMed versions
 * if any of the ROM code is being replaced by a FLASH equivalent.
 */
portBASE_TYPE xPortStartScheduler( portBASE_TYPE xUseKernelConfigurationChecks );



#endif /* PORTABLE_H */

