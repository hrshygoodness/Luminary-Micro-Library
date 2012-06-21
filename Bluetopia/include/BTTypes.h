/*****< bttypes.h >************************************************************/
/*      Copyright 2000 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTTYPES - Common Bluetooth Defined Types.                                 */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   08/20/00  D. Lange       Initial creation.                               */
/*   12/07/07  D. Mason       Changes for BT 2.1                              */
/******************************************************************************/
#ifndef __BTTYPESH__
#define __BTTYPESH__

   /* Miscellaneous defined type declarations.                          */

   /* Definitions for compilers that required structure to be explicitly*/
   /* declared as packed.                                               */
#ifdef rvmdk
   #define __PACKED_STRUCT_BEGIN__   __packed
#else
   #define __PACKED_STRUCT_BEGIN__
#endif

#if (defined(ccs) || defined(codered) || defined(gcc) || defined(rvmdk) || defined(sourcerygxx))
   #define __PACKED_STRUCT_END__     __attribute__ ((packed))
#else
   #define __PACKED_STRUCT_END__
#endif

#include "BaseTypes.h"
#include "BTBTypes.h"

#endif
