/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  BVENDAPI.h - Stellaris(R) Bluetooth Chipset Vendor Specific API Type      */
/*               Definitions, Constants and Prototypes for Stonestreet One,   */
/*               LLC Bluetopia(R) Bluetooth Protocol Stack.                   */
/*                                                                            */
/******************************************************************************/
#ifndef __BVENDAPIH__
#define __BVENDAPIH__

#include "SS1BTPS.h"            /* Bluetopia API Prototypes/Constants.        */

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality that needs to be performed before   */
   /* the HCI Communications layer is opened.  This function is called  */
   /* immediately prior to calling the initialization of the HCI        */
   /* Communications layer.  This function should return a BOOLEAN TRUE */
   /* indicating successful completion or should return FALSE to        */
   /* indicate unsuccessful completion.  If an error is returned the    */
   /* stack will fail the initialization process.                       */
   /* * NOTE * The parameter passed to this function is the exact       */
   /*          same parameter that was passed to BSC_Initialize() for   */
   /*          stack initialization.  If this function changes any      */
   /*          members that this pointer points to, it will change the  */
   /*          structure that was originally passed.                    */
   /* * NOTE * No HCI communication calls are possible to be used in    */
   /*          this function because the driver has not been initialized*/
   /*          at the time this function is called.                     */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIOpen(HCI_DriverInformation_t *HCI_DriverInformation);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIOpen_t)(HCI_DriverInformation_t *HCI_DriverInformation);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the HCI Communications layer  */
   /* is initialized (the driver only).  This function is called        */
   /* immediately after returning from the initialization of the HCI    */
   /* Communications layer (HCI Driver).  This function should return a */
   /* BOOLEAN TRUE indicating successful completion or should return    */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * No HCI layer function calls are possible to be used in   */
   /*          this function because the actual stack has not been      */
   /*          initialized at this point.  The only initialization that */
   /*          has occurred is with the HCI Driver (hence the HCI       */
   /*          Driver ID that is passed to this function).              */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIOpen(unsigned int HCIDriverID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIOpen_t)(unsigned int HCIDriverID);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functions after the HCI Communications layer AND  */
   /* the HCI Stack layer has been initialized.  This function is called*/
   /* after all HCI functionality is established, but before the initial*/
   /* HCI Reset is sent to the stack.  The function should return a     */
   /* BOOLEAN TRUE to indicate successful completion or should return   */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time       */
   /*          (hence the HCI Driver ID and the Bluetooth Stack ID      */
   /*          passed to this function).                                */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIReset_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the HCI layer has issued any  */
   /* HCI Reset as part of the initialization.  This function is called */
   /* after all HCI functionality is established, just after the initial*/
   /* HCI Reset is sent to the stack.  The function should return a     */
   /* BOOLEAN TRUE to indicate successful completion or should return   */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time (hence*/
   /*          the HCI Driver ID and the Bluetooth Stack ID passed to   */
   /*          this function).                                          */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIReset_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which would is used to implement any needed Bluetooth    */
   /* device vendor specific functionality before the HCI layer is      */
   /* closed.  This function is called at the start of the HCI_Cleanup()*/
   /* function (before the HCI layer is closed), at which time all HCI  */
   /* functions are still operational.  The caller is NOT able to call  */
   /* any other stack functions other than the HCI layer and HCI Driver */
   /* layer functions because the stack is being shutdown (i.e.         */
   /* something has called BSC_Shutdown()).  The caller is free to      */
   /* return either success (TRUE) or failure (FALSE), however, it will */
   /* not circumvent the closing down of the stack or of the HCI layer  */
   /* or HCI Driver (i.e. the stack ignores the return value from this  */
   /* function).                                                        */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time (hence*/
   /*          the HCI Driver ID and the Bluetooth Stack ID passed to   */
   /*          this function).                                          */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIClose(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIClose_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the entire Bluetooth Stack is */
   /* closed.  This function is called during the HCI_Cleanup()         */
   /* function, after the HCI Driver has been closed.  The caller is    */
   /* free return either success (TRUE) or failure (FALSE), however, it */
   /* will not circumvent the closing down of the stack as all layers   */
   /* have already been closed.                                         */
   /* * NOTE * No Stack calls are possible in this function because the */
   /*          entire stack has been closed down at the time this       */
   /*          function is called.                                      */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIClose(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIClose_t)(void);
#endif

#endif
