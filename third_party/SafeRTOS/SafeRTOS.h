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

#ifndef INC_SAFERTOS_H
#define INC_SAFERTOS_H


/*
 * Include the generic headers required for the SafeRTOS port being used.
 */
#include <stddef.h> /*lint !e537 This seems to also be included from library files with some compilers.  This is not a problem here. */

/* Basic SafeRTOS definitions. */
#include "SafeRTOS/projdefs.h"

/* Application specific configuration options. */
#include "SafeRTOS/SafeRTOSConfig.h"

/* Definitions specific to the port being used. */
#include "SafeRTOS/portable.h"


#endif /* INC_SAFERTOS_H */
