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
