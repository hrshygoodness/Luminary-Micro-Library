/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  HCITRANS.h - Stellaris(R) HCI Transport Layer Type Definitions, Constants,*/
/*               and Prototypes for use with Stonestreet One, LLC Bluetopia(R)*/
/*               Bluetooth Protocol Stack.                                    */
/*                                                                            */
/******************************************************************************/
#ifndef __HCITRANSH__
#define __HCITRANSH__

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "HCITypes.h"           /* Bluetooth HCI Type Definitions/Constants.  */

  /* If building with a C++ compiler, make all of the definitions in    */
  /* this header have a C binding.                                      */
#ifdef __cplusplus

extern "C"
{

#endif

#define HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT (-1)       /* Denotes that the   */
                                                        /* there was an error */
                                                        /* opening the        */
                                                        /* transport layer.   */

#define HCITR_ERROR_READING_FROM_PORT        (-2)       /* Denotes that an    */
                                                        /* error occured      */
                                                        /* reading from the   */
                                                        /* transport layer    */
                                                        /* port.              */

#define HCITR_ERROR_WRITING_TO_PORT          (-3)       /* Denotes that an    */
                                                        /* error occured      */
                                                        /* writing to the     */
                                                        /* transport layer    */
                                                        /* port.              */

   /* The following declared type represents the Prototype Function for */
   /* an HCI Transport Driver Data Callback for COM data.  This function*/
   /* will be called whenever HCI Packet Information has been received  */
   /* by the HCI Transport Driver.  This function passes to the caller  */
   /* the actual data that was received (Length of Data followed by a   */
   /* pointer to the data).  The caller is free to use the contents of  */
   /* the HCI Data ONLY in the context of this callback.  If the caller */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e. this    */
   /* function DOES NOT have be reentrant).  It should be noted that    */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* Packet Processing (Receiving) will be suspended while this        */
   /* function call is outstanding).                                    */
   /* * NOTE * The type of data (Event, ACL, SCO, etc.) is not passed to*/
   /*          the caller in this callback because it is assumed that   */
   /*          this information is contained in the Data Stream being   */
   /*          passed to the caller.                                    */
typedef void (BTPSAPI *HCITR_COMDataCallback_t)(unsigned int HCITransportID, unsigned int DataLength, unsigned char *DataBuffer, unsigned long CallbackParameter);

   /* The following function is responsible for opening the HCI         */
   /* Transport layer that will be used by Bluetopia to send and receive*/
   /* COM (Serial) data.  This function must be successfully issued in  */
   /* order for Bluetopia to function.  This function accepts as its    */
   /* parameter the HCI COM Transport COM Information that is to be used*/
   /* to open the port.  The final two parameters specify the HCI       */
   /* Transport Data Callback and Callback Parameter (respectively) that*/
   /* is to be called when data is received from the UART.  A successful*/
   /* call to this function will return a non-zero, positive value which*/
   /* specifies the HCITransportID that is used with the remaining      */
   /* transport functions in this module.  This function returns a      */
   /* negative return value to signify an error.                        */
int BTPSAPI HCITR_COMOpen(HCI_COMMDriverInformation_t *COMMDriverInformation, HCITR_COMDataCallback_t COMDataCallback, unsigned long CallbackParameter);

   /* The following function is responsible for closing the specific    */
   /* HCI Transport layer that was opened via a successful call to the  */
   /* HCITR_COMOpen() function (specified by the first parameter).      */
   /* Bluetopia makes a call to this function whenever an either        */
   /* Bluetopia is closed, or an error occurs during initialization and */
   /* the driver has been opened (and ONLY in this case).  Once this    */
   /* function completes, the transport layer that was closed will no   */
   /* longer process received data until the transport layer is         */
   /* Re-Opened by calling the HCITR_COMOpen() function.                */
   /* * NOTE * This function *MUST* close the specified COM Port.       */
   /*          This module will then call the registered COM Data       */
   /*          Callback function with zero as the data length and NULL  */
   /*          as the data pointer.  This will signify to the HCI       */
   /*          Driver that this module is completely finished with the  */
   /*          port and information and (more importantly) that NO      */
   /*          further data callbacks will be issued.  In other words   */
   /*          the very last data callback that is issued from this     */
   /*          module *MUST* be a data callback specifying zero and NULL*/
   /*          for the data length and data buffer (respectively).      */
void BTPSAPI HCITR_COMClose(unsigned int HCITransportID);

   /* The following function is responsible for instructing the         */
   /* specified HCI Transport layer (first parameter) that was opened   */
   /* via a successful call to the HCITR_COMOpen() function to          */
   /* reconfigure itself with the specified information.                */
   /* * NOTE * This function does not close the HCI Transport specified */
   /*          by HCI Transport ID, it merely reconfigures the          */
   /*          transport.  This means that the HCI Transport specified  */
   /*          by HCI Transport ID is still valid until it is closed    */
   /*          via the HCI_COMClose() function.                         */
void BTPSAPI HCITR_COMReconfigure(unsigned int HCITransportID, HCI_Driver_Reconfigure_Data_t *DriverReconfigureData);

   /* The following function is provided to allow a mechanism for       */
   /* modules to force the processing of incoming COM Data.             */
   /* * NOTE * This function is only applicable in device stacks that   */
   /*          are non-threaded.  This function has no effect for device*/
   /*          stacks that are operating in threaded environments.      */
void BTPSAPI HCITR_COMProcess(unsigned int HCITransportID);

   /* The following function is responsible for actually sending data   */
   /* through the opened HCI Transport layer (specified by the first    */
   /* parameter).  Bluetopia uses this function to send formatted HCI   */
   /* packets to the attached Bluetooth Device.  The second parameter to*/
   /* this function specifies the number of bytes pointed to by the     */
   /* third parameter that are to be sent to the Bluetooth Device.  This*/
   /* function returns a zero if the all data was transfered sucessfully*/
   /* or a negetive value if an error occurred.  This function MUST NOT */
   /* return until all of the data is sent (or an error condition       */
   /* occurs).  Bluetopia WILL NOT attempt to call this function        */
   /* repeatedly if data fails to be delivered.  This function will     */
   /* block until it has either buffered the specified data or sent all */
   /* of the specified data to the Bluetooth Device.                    */
   /* * NOTE * The type of data (Command, ACL, SCO, etc.) is NOT passed */
   /*          to this function because it is assumed that this         */
   /*          information is contained in the Data Stream being passed */
   /*          to this function.                                        */
int BTPSAPI HCITR_COMWrite(unsigned int HCITransportID, unsigned int Length, unsigned char *Buffer);

   /* The following function is called when an invalid start of packet  */
   /* byte is received.  This function can this handle this case as     */
   /* needed.  The first parameter is the Transport ID that was         */
   /* from a successfull call to HCITR_COMWrite and the second          */
   /* parameter is the invalid start of packet byte that was received.  */
void BTPSAPI HCITR_InvalidStart_Callback(unsigned int HCITransportID, unsigned char Data);

   /* Mark the end of the C bindings section for C++ compilers.         */
#ifdef __cplusplus

}

#endif

#endif
