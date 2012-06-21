/*****< btvsapi.h >************************************************************/
/*      Copyright 2011 Stonestreet One.                                       */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTVSAPI - Vendor specific functions/definitions/constants used to define  */
/*            a set of vendor specific functions supported for a specific     */
/*            hardware platform.                                              */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/20/11  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTVSAPIH__
#define __BTVSAPIH__

#include "SS1BTPS.h"            /* Bluetopia API Prototypes/Constants.        */

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the Bluetooth UART for the Local */
   /* Bluetooth Device specified by the Bluetooth Protocol Stack that   */
   /* is specified by the Bluetooth Protocol Stack ID. The second       */
   /* parameter specifies the new baud rate to set.  This change        */
   /* encompasses both changing the speed of the Bluetooth chip (by     */
   /* issuing the correct commands) and then, if successful, informing  */
   /* the HCI Driver of the change (so the driver can communicate with  */
   /* the Bluetooth device at the new baud rate).  This function returns*/
   /* zero if successful or a negative return error code if there was   */
   /* an error.                                                         */
BTPSAPI_DECLARATION int BTPSAPI VS_Update_UART_Baud_Rate(unsigned int BluetoothStackID, DWord_t BaudRate);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_VS_Update_UART_Baud_Rate_t)(unsigned int BluetoothStackID, DWord_t BaudRate);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which is used to change the output power for the Local   */
   /* Bluetooth Device specified by the Bluetooth Protocol Stack that   */
   /* is specified by the Bluetooth Protocol Stack ID. The second       */
   /* parameter is the max output power to set. This function returns   */
   /* zero if successful or a negative return error code if there was   */
   /* an error.                                                         */
   /* * NOTE * The maximum output power is specified from 0 to 12 and   */
   /*          it specifies 4 dBm steps.                                */
BTPSAPI_DECLARATION int BTPSAPI VS_Set_Max_Output_Power(unsigned int BluetoothStackID, Byte_t MaxPower);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_VS_Set_Max_Output_Power_t)(unsigned int BluetoothStackID, Byte_t MaxPower);
#endif

#endif
