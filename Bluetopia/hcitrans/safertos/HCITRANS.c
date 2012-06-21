/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  HCITRANS.c - Stellaris(R) HCI Transport Layer Implementation for use with */
/*               Stonestreet One, LLC Bluetopia(R) Bluetooth Protocol Stack.  */
/*                                                                            */
/******************************************************************************/
#include "BTPSKRNL.h"            /* Bluetooth Kernel Prootypes/Constants.     */
#include "HCITRANS.h"            /* HCI Transport Prototypes/Constants.       */
#include "HCITypes.h"            /* HCI Transport Types.                      */

#include "bt_ucfg.h"             /* UART Configuration MACRO's/constants.     */

#include "SafeRTOS/SafeRTOS_API.h"  /* SafeRTOS Kernal Prototypes/Constants.  */

#include "inc/hw_gpio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

#define TRANSPORT_ID                                                  1

   /* HCI Transport Buffer and Flow Control Configuration constants.    */
#ifndef DEFAULT_INPUT_BUFFER_SIZE

   #define DEFAULT_INPUT_BUFFER_SIZE                               1024

#endif

#ifndef DEFAULT_OUTPUT_BUFFER_SIZE

   #define DEFAULT_OUTPUT_BUFFER_SIZE                               512

#endif

#ifndef DEFAULT_XOFF_LIMIT

   #define DEFAULT_XOFF_LIMIT                                       128

#endif

#ifndef DEFAULT_XON_LIMIT

   #define DEFAULT_XON_LIMIT                                        512

#endif

#define FLOW_OFF                                                   0xFF
#define FLOW_ON                                                    0x00

#define UART_CONTEXT_FLAG_OPEN_STATE                             0x0001
#define UART_CONTEXT_FLAG_FLOW_CONTROL_ENABLED                   0x0002
#define UART_CONTEXT_FLAG_RX_FLOW_ENABLED                        0x0004
#define UART_CONTEXT_FLAG_TX_FLOW_ENABLED                        0x0008
#define UART_CONTEXT_FLAG_RX_OVERRUN                             0x0010
#define UART_CONTEXT_FLAG_RX_ERROR                               0x0020

typedef struct _tagUartContext_t
{
   unsigned char  ID;
   unsigned long  Base;
   unsigned long  IntBase;
   unsigned short FlowInfo;
   unsigned char  RxBuffer[DEFAULT_INPUT_BUFFER_SIZE];
   int            RxBufferSize;
   int            RxBytesFree;
   int            RxInIndex;
   int            RxOutIndex;
   int            XOffLimit;
   int            XOnLimit;
   unsigned char  TxBuffer[DEFAULT_OUTPUT_BUFFER_SIZE];
   int            TxBufferSize;
   int            TxBytesFree;
   int            TxInIndex;
   int            TxOutIndex;
   unsigned char  Flags;
} UartContext_t;

   /* Register base definitions.                                        */
#define BT_RTS_BASE              (HCI_UART_RTS_GPIO_BASE + (HCI_UART_PIN_RTS << 2))

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static unsigned int HCITransportOpen;

static UartContext_t  UartContext;
static Event_t        RxDataEvent;
static ThreadHandle_t Handle = NULL;
static Boolean_t      RxThreadDeleted = FALSE;

   /* COM Data Callback Function and Callback Parameter information.    */
static HCITR_COMDataCallback_t _COMDataCallback;
static unsigned long           _COMCallbackParameter;

   /* Local Function Prototypes.                                        */
static void FlushRxFIFO(unsigned long Base);
static int  GetUartChars(unsigned long Base, unsigned char *Dest, int MaxChars);
static void RxInterrupt(void);
static void TxInterrupt(void);
static void *RxThread(void *Param);

   /* The following function is used to unload all of the characters in */
   /* the receive FIFO.                                                 */
static void FlushRxFIFO(unsigned long Base)
{
   unsigned long Dummy;

   while(!(HWREG(Base + UART_O_FR) & UART_FR_RXFE))
   {
      /* Remove the character from the FIFO.                            */
      Dummy = HWREG(Base + UART_O_DR);
      if(Dummy)
         Dummy = 0;
   }
}

   /* The following function is used to remove characters from the input*/
   /* FIFO and place the characters sequentially in a buffer that is    */
   /* pointer to by Dest.  The MaxChars parameter defines the number of */
   /* bytes that the destination buffer can hold.  The function returns */
   /* the number of characters that were moved to the destination.      */
static int GetUartChars(unsigned long Base, unsigned char *Dest, int MaxChars)
{
   int ret_val;

   /* Verify that the parameters passed in appear valid.                */
   if((Base) && (Dest))
   {
      /* Initialize the count of the characters that are processed.     */
      ret_val = 0;

      /* Check to see if the Rx FIFO is empty.                          */
      while((MaxChars) && (!(HWREG(Base + UART_O_FR) & UART_FR_RXFE)))
      {
         /* Move the character from the FIFO to the Destination Buffer. */
         *Dest++ = (unsigned char)HWREG(Base + UART_O_DR);
         ret_val++;
         MaxChars--;
      }
   }
   else
      ret_val = -1;

   return(ret_val);
}

   /* The following function is the Interrupt Service Routine for the   */
   /* UART RX interrupt.                                                */
static void RxInterrupt(void)
{
   int            BufferBytes;
   int            ProcessCount;
   int            Processed;
   unsigned char  NotDone;

   /* We will process the received data until there is no more.  When   */
   /* all of the data has been processed, the NoDone flag will be       */
   /* cleared.                                                          */
   NotDone = 1;
   while(NotDone)
   {
      /* We will process the received data in a tight loop.  The number */
      /* of bytes that we can place in the queue in a sequential order  */
      /* will be limited by the number of bytes before buffer will wrap */
      /* and the number of bytes before we reach the XOff limit or the  */
      /* buffer becomes totally full.  Of these three limits, we will   */
      /* need to pick the smallest value.                               */
      BufferBytes = (UartContext.RxBytesFree - UartContext.XOffLimit);
      if(BufferBytes <= 0)
         BufferBytes = UartContext.RxBytesFree;

      /* Check to see if there is buffer space to receive the data.     */
      if(BufferBytes)
      {
         /* Determine the number of Bytes till the buffer wrapps.       */
         ProcessCount = (UartContext.RxBufferSize - UartContext.RxInIndex);

         /* Process the small of the two values.                        */
         if(ProcessCount > BufferBytes)
            ProcessCount = BufferBytes;

         /* Read as may characters as possible.                         */
         Processed = GetUartChars(UartContext.Base, &UartContext.RxBuffer[UartContext.RxInIndex], ProcessCount);
         if(Processed > 0)
         {
            /* Increament the In Index by the nunber of characters that */
            /* were placed in the Rx Buffer.  If we processed all that  */
            /* we indicated that we could take, then handle this        */
            /* condition.                                               */
            UartContext.RxInIndex   += Processed;
            UartContext.RxBytesFree -= Processed;
            if(Processed == ProcessCount)
            {
               /* Check to see if we have reached the end of the Buffer */
               /* and need to loop back to the beginning.               */
               if(UartContext.RxInIndex == UartContext.RxBufferSize)
                  UartContext.RxInIndex = 0;
               else
               {
                  /* Check to see if we should perform Flow Control or  */
                  /* wrap the buffer.                                   */
                  /* * NOTE * If flow control is not enabled, then the  */
                  /*          XOffLimit will be set to the Rx Buffer    */
                  /*          size and thus we will loop the In pointer */
                  /*          and not get here.                         */
                  if(UartContext.RxBytesFree == UartContext.XOffLimit)
                  {
                     HWREG(BT_RTS_BASE)    = FLOW_OFF;
                     UartContext.FlowInfo &= ~UART_CONTEXT_FLAG_RX_FLOW_ENABLED;
                  }
               }
            }
            else
               NotDone = 0;
         }
         else
            NotDone = 0;
      }
      else
      {
         /* Flag that we have encountered an RX Overrun.  Disable the Rx*/
         /* interrupt and Flag the error.                               */
         UARTIntEnable(UartContext.Base, UART_INT_RT);
         UartContext.Flags |= UART_CONTEXT_FLAG_RX_OVERRUN;
         NotDone            = 0;
      }
   }

   /* Signal the reception of some data.                             */
   BTPS_INT_SetEvent(RxDataEvent);
}

   /* The following function is the FIFO Primer and Interrupt Service   */
   /* Routine for the UART TX interrupt.                                */
static void TxInterrupt(void)
{
   int            Count;
   int            Processed;
   Boolean_t      Done;
   unsigned char *Ch;

   /* Check to see if there are characters in the Transmit Buffer.      */
   if(UartContext.TxBytesFree != UartContext.TxBufferSize)
   {
      Done = FALSE;
      while(!Done)
      {
         /* Get the maximum number of characters that can be sent to the*/
         /* FIFO before the buffer needs to be wrapped.                 */
         /* * NOTE * This calculation is has been reduced for speed.    */
         /*          The first test is to determine if the number of    */
         /*          available characters are greater than the number if*/
         /*          characters till the buffer is wrapped.  The number */
         /*          of available characters is determined by           */
         /*          (BufferSize-BytesFree) and characters till wrap is */
         /*          determined by (BufferSize-OutIndex).  Since both   */
         /*          values are determined by subtracting from          */
         /*          BufferSize, the value that we need can be          */
         /*          determined without performing the subtraction.     */
         Count     = (UartContext.TxBytesFree < UartContext.TxOutIndex)?(UartContext.TxBufferSize-UartContext.TxOutIndex):(UartContext.TxBufferSize-UartContext.TxBytesFree);
         Ch        = &UartContext.TxBuffer[UartContext.TxOutIndex];
         Processed = 0;
         while((!(HWREG(UartContext.Base + UART_O_FR) & UART_FR_TXFF)) && (Count--))
         {
            HWREG(UartContext.Base + UART_O_DR) = (unsigned long)*Ch++;
            Processed++;
         }

         /* Check to see if we processed any of the characters in the   */
         /* input buffer.                                               */
         if(Processed)
         {
            /* Update the indices and counts.                           */
            UartContext.TxBytesFree += Processed;
            UartContext.TxOutIndex  += Processed;
            if(UartContext.TxOutIndex == UartContext.TxBufferSize)
            {
               /* Wrap the index back to the beginning and check to see */
               /* if we can continue to send more data.                 */
               UartContext.TxOutIndex = 0;
            }
         }
         else
            Done = TRUE;
      }
   }

   /* If there are no more bytes in the queue then disable the transmit */
   /* interrupt.                                                        */
   if(UartContext.TxBytesFree == UartContext.TxBufferSize)
      HWREG(UartContext.Base + UART_O_IM) &= ~UART_IM_TXIM;
}

   /* The following thread is used to process the data that has been    */
   /* received from the UART and placed in the receive buffer.          */
static void *RxThread(void *Param)
{
   int       MaxWrite;
   int       Count;

   /* This thread will loop forever.                                    */
   while(!RxThreadDeleted)
   {
      /* Check to see if there are any characters in the receive buffer.*/
      Count = (UartContext.RxBufferSize - UartContext.RxBytesFree);
      if(!Count)
      {
         /* Wait for a Recevied Character event;                        */
         BTPS_WaitEvent(RxDataEvent, BTPS_INFINITE_WAIT);
         if(!RxThreadDeleted)
            BTPS_ResetEvent(RxDataEvent);
         else
            break;

         Count = (UartContext.RxBufferSize - UartContext.RxBytesFree);
      }

      if(Count)
      {
         /* Determine the maximum number of characters that we can send */
         /* before we reach the end of the buffer.  We need to process  */
         /* the smaller of the max characters of the number of          */
         /* characters that are in the buffer.                          */
         MaxWrite = (UartContext.RxBufferSize-UartContext.RxOutIndex);
         Count    = (MaxWrite < Count)?MaxWrite:Count;

         /* Call the upper layer back with the data.                    */
         if(_COMDataCallback)
            (*_COMDataCallback)(TRANSPORT_ID, Count, &UartContext.RxBuffer[UartContext.RxOutIndex], _COMCallbackParameter);

         /* Adjust the Out Index and handle any looping.                */
         UartContext.RxOutIndex += Count;
         if(UartContext.RxOutIndex >= UartContext.RxBufferSize)
            UartContext.RxOutIndex = 0;

         /* Enter a critical section by disabling the UART interrupt    */
         /* since the UART interrupts access and change the UartContext */
         /* structure (which we are about to change).                   */
         MAP_IntDisable(UartContext.IntBase);

         /* Credit the amount that was sent.                            */
         UartContext.RxBytesFree += Count;

         /* Check to see if we need to recover from a Buffer Overflow.  */
         if(UartContext.Flags & UART_CONTEXT_FLAG_RX_OVERRUN)
         {
            /* Clear the Overrun Flag and re-enabled the interrupt.     */
            UartContext.Flags &= ~UART_CONTEXT_FLAG_RX_OVERRUN;
            MAP_UARTIntEnable(UartContext.Base, UART_INT_RX | UART_INT_RT);
         }

         /* Check to se if we need to re-enable Flow Control.           */
         if(!(UartContext.FlowInfo & UART_CONTEXT_FLAG_RX_FLOW_ENABLED) && (UartContext.RxBytesFree > UartContext.XOnLimit))
         {
            HWREG(BT_RTS_BASE)    = FLOW_ON;
            UartContext.FlowInfo |= UART_CONTEXT_FLAG_RX_FLOW_ENABLED;
         }

         /* Re-enable UART interrupts to exit from the critical section.*/
         MAP_IntEnable(UartContext.IntBase);
      }
   }

   return(NULL);
}

   /* The following is the ISR for the UART serial device. This         */
   /* determines the cause of the UART interrupt (Tx or Rx) and then    */
   /* calls an appropriate handler function.                            */
void HCITR_UARTIntHandler(void)
{
   unsigned long  Reg;

   /* Clear the Interrupt Pending Flag for this UART.                   */
   Reg                                  = HWREG(UartContext.Base + UART_O_MIS);
   HWREG(UartContext.Base + UART_O_ICR) = Reg;

   /* Check to see if data is available in the Receive Buffer.          */
   if(Reg & (UART_MIS_RXMIS | UART_MIS_RTMIS))
      RxInterrupt();

   /* Check to see if we may now write more bytes to transmit into the  */
   /* UART.                                                             */
   if(Reg & UART_MIS_TXMIS)
      TxInterrupt();
}

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
   /* negative return value to signify an error or the Transport ID on  */
   /* success.                                                          */
int BTPSAPI HCITR_COMOpen(HCI_COMMDriverInformation_t *COMMDriverInformation, HCITR_COMDataCallback_t COMDataCallback, unsigned long CallbackParameter)
{
   int ret_val;

   /* First, make sure that the port is not already open and make sure  */
   /* that valid COMM Driver Information was specified.                 */
   if((!HCITransportOpen) && (COMMDriverInformation) && (COMDataCallback))
   {
      /* Initialize the return value for success.                       */
      ret_val               = TRANSPORT_ID;

      /* Note the COM Callback information.                             */
      _COMDataCallback      = COMDataCallback;
      _COMCallbackParameter = CallbackParameter;

      /* Initialize the UART Context Structure.                         */
      BTPS_MemInitialize(&UartContext, 0, sizeof(UartContext_t));

      UartContext.Base         = HCI_UART_BASE;
      UartContext.IntBase      = HCI_UART_INT;
      UartContext.ID           = 1;
      UartContext.FlowInfo     = UART_CONTEXT_FLAG_FLOW_CONTROL_ENABLED;
      UartContext.XOnLimit     = DEFAULT_XON_LIMIT;
      UartContext.XOffLimit    = DEFAULT_XOFF_LIMIT;
      UartContext.RxBufferSize = DEFAULT_INPUT_BUFFER_SIZE;
      UartContext.RxBytesFree  = DEFAULT_INPUT_BUFFER_SIZE;
      UartContext.TxBufferSize = DEFAULT_OUTPUT_BUFFER_SIZE;
      UartContext.TxBytesFree  = DEFAULT_OUTPUT_BUFFER_SIZE;

      /* Flag that the Rx Thread should not delete itself.              */
      RxThreadDeleted          = FALSE;

      /* Check to see if this is the first time that the port has been  */
      /* opened.                                                        */
      if(!Handle)
      {
         /* Configure the UART module and the GPIO pins used by the     */
         /* UART.                                                       */
         MAP_SysCtlPeripheralEnable(HCI_UART_GPIO_PERIPH);
         MAP_SysCtlPeripheralEnable(HCI_UART_RTS_GPIO_PERIPH);
         MAP_SysCtlPeripheralEnable(HCI_UART_CTS_GPIO_PERIPH);
         MAP_SysCtlPeripheralEnable(HCI_UART_PERIPH);

         MAP_GPIOPinConfigure(HCI_PIN_CONFIGURE_UART_RX);
         MAP_GPIOPinConfigure(HCI_PIN_CONFIGURE_UART_TX);
         MAP_GPIOPinConfigure(HCI_PIN_CONFIGURE_UART_RTS);
         MAP_GPIOPinConfigure(HCI_PIN_CONFIGURE_UART_CTS);

         MAP_GPIOPinTypeUART(HCI_UART_GPIO_BASE, HCI_UART_PIN_RX | HCI_UART_PIN_TX);
         MAP_GPIOPinTypeUART(HCI_UART_RTS_GPIO_BASE, HCI_UART_PIN_RTS);
         MAP_GPIOPinTypeUART(HCI_UART_CTS_GPIO_BASE, HCI_UART_PIN_CTS);

         UARTFlowControlSet(UartContext.Base, UART_FLOWCONTROL_RX | UART_FLOWCONTROL_TX);

         /* Create an Event that will be used to signal that data has   */
         /* arrived.                                                    */
         RxDataEvent = BTPS_CreateEvent(FALSE);
         if(RxDataEvent)
         {
            /* Create a thread that will process the received data.     */
            Handle = BTPS_CreateThread(RxThread, 1600, NULL);
            if(!Handle)
            {
               BTPS_CloseEvent(RxDataEvent);
               ret_val = HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT;
            }
         }
         else
            ret_val = HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT;
      }

      /* If there was no error, then continue to setup the port.        */
      if(ret_val != HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT)
      {
         /* Configure UART Baud Rate and Interrupts.                    */
         MAP_UARTConfigSetExpClk(UartContext.Base, MAP_SysCtlClockGet(), COMMDriverInformation->BaudRate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

         /* SafeRTOS requires RTOS-aware int handlers to be priority    */
         /* value 5 or greater                                          */
         MAP_IntPrioritySet(UartContext.IntBase, 6 << 5);

         MAP_IntEnable(UartContext.IntBase);
         MAP_UARTIntEnable(UartContext.Base, UART_INT_RX | UART_INT_RT);
         UartContext.Flags |= UART_CONTEXT_FLAG_RX_FLOW_ENABLED;

         /* Clear any data that is in the Buffer.                       */
         FlushRxFIFO(UartContext.Base);

         /* Bring the Bluetooth Device out of Reset.                    */
         MAP_GPIOPinWrite(HCI_RESET_BASE, HCI_RESET_PIN, HCI_RESET_PIN);

         /* Check to see if we need to delay after opening the COM Port.*/
         if(COMMDriverInformation->InitializationDelay)
            BTPS_Delay(COMMDriverInformation->InitializationDelay);

         /* Flag that the HCI Transport is open.                        */
         HCITransportOpen = 1;
      }
   }
   else
      ret_val = HCITR_ERROR_UNABLE_TO_OPEN_TRANSPORT;

   return(ret_val);
}

   /* The following function is responsible for closing the the specific*/
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
void BTPSAPI HCITR_COMClose(unsigned int HCITransportID)
{
   HCITR_COMDataCallback_t COMDataCallback;
   unsigned long           CallbackParameter;

   /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (HCITransportOpen))
   {
      /* Disable the UART Interrupts.                                   */
      MAP_UARTIntDisable(UartContext.Base, UART_INT_RX | UART_INT_RT);
      MAP_IntDisable(UartContext.IntBase);

      /* Place the Bluetooth Device in Reset.                           */
      MAP_GPIOPinWrite(HCI_RESET_BASE, HCI_RESET_PIN, 0);

      /* Note the Callback information.                                 */
      COMDataCallback   = _COMDataCallback;
      CallbackParameter = _COMCallbackParameter;

      /* Flag that the HCI Transport is no longer open.                 */
      HCITransportOpen  = 0;

      /* Flag that there is no callback information present.            */
      _COMDataCallback      = NULL;
      _COMCallbackParameter = 0;

      /* Flag that the RxThread is deleted.                             */
      Handle            = NULL;

      /* Flag that the Rx Thread should delete itself.                  */
      RxThreadDeleted   = TRUE;
      BTPS_SetEvent(RxDataEvent);

      /* Flag that the RxThread is deleted.                             */
      Handle = NULL;

      /* Delay while the RxThread exits.                                */
      BTPS_Delay(5);

      /* All finished, perform the callback to let the upper layer know */
      /* that this module will no longer issue data callbacks and is    */
      /* completely cleaned up.                                         */
      if(COMDataCallback)
         (*COMDataCallback)(HCITransportID, 0, NULL, CallbackParameter);

      /* Close the RxData event.                                        */
      BTPS_CloseEvent(RxDataEvent);
   }
}

   /* The following function is responsible for instructing the         */
   /* specified HCI Transport layer (first parameter) that was opened   */
   /* via a successful call to the HCITR_COMOpen() function to          */
   /* reconfigure itself with the specified information.  This          */
   /* information is completely opaque to the upper layers and is passed*/
   /* through the HCI Driver layer to the transport untouched.  It is   */
   /* the responsibility of the HCI Transport driver writer to define   */
   /* the contents of this member (or completely ignore it).            */
   /* * NOTE * This function does not close the HCI Transport specified */
   /*          by HCI Transport ID, it merely reconfigures the          */
   /*          transport.  This means that the HCI Transport specified  */
   /*          by HCI Transport ID is still valid until it is closed    */
   /*          via the HCI_COMClose() function.                         */
void BTPSAPI HCITR_COMReconfigure(unsigned int HCITransportID, HCI_Driver_Reconfigure_Data_t *DriverReconfigureData)
{
   long                          BaudRate;

   /* Check to make sure that the specified Transport ID is valid.      */
   if((DriverReconfigureData) && (HCITransportID == TRANSPORT_ID) && (HCITransportOpen))
   {
      /* Change the UART baud rate.                                     */
      if(DriverReconfigureData->ReconfigureCommand == HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_PARAMETERS)
      {
         BaudRate = (long)DriverReconfigureData->ReconfigureData;
         MAP_UARTConfigSetExpClk(UartContext.Base, MAP_SysCtlClockGet(), BaudRate, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
      }
   }
}

   /* The following function is provided to allow a mechanism for       */
   /* modules to force the processing of incoming COM Data.             */
   /* * NOTE * This function is only applicable in device stacks that   */
   /*          are non-threaded.  This function has no effect for device*/
   /*          stacks that are operating in threaded environments.      */
void BTPSAPI HCITR_COMProcess(unsigned int HCITransportID)
{
   /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (HCITransportOpen))
   {
   }
}

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
int BTPSAPI HCITR_COMWrite(unsigned int HCITransportID, unsigned int Length, unsigned char *Buffer)
{
   int ret_val;
   int Count;

   /* Check to make sure that the specified Transport ID is valid and   */
   /* the output buffer appears to be valid as well.                    */
   if((HCITransportID == TRANSPORT_ID) && (HCITransportOpen) && (Length) && (Buffer))
   {
      /* Delay and poll until there is enough room in the Tx Buffer (in */
      /* the UartContext structure) to hold the data we are trying to   */
      /* transmit.                                                      */
      while(UartContext.TxBytesFree < Length)
         BTPS_Delay(10);

      /* Process all of the data.                                       */
      while(Length)
      {
         /* The data may have to be copied in 2 phases.  Calculate the  */
         /* number of character that can be placed in the buffer before */
         /* the buffer must be wrapped.                                 */
         Count = (UartContext.TxBufferSize-UartContext.TxInIndex);
         Count = (Count > Length)?Length:Count;
         BTPS_MemCopy(&(UartContext.TxBuffer[UartContext.TxInIndex]), Buffer, Count);

         /* Update the number of free bytes in the buffer.  Since this  */
         /* count can also be updated in the interrupt routine, we will */
         /* have have to update this with interrupts disabled.          */
         MAP_IntDisable(UartContext.IntBase);
         UartContext.TxBytesFree -= Count;
         MAP_IntEnable(UartContext.IntBase);

         /* Adjust the count and index values.                          */
         Buffer                += Count;
         Length                -= Count;
         UartContext.TxInIndex += Count;
         if(UartContext.TxInIndex >= UartContext.TxBufferSize)
            UartContext.TxInIndex = 0;
      }

      /* Check to see if we need to prime the transmitter.              */
      if(!(HWREG(UartContext.Base + UART_O_IM) & UART_IM_TXIM))
      {
         /* Now that the data is in the input buffer, check to see if we*/
         /* need to enable the interrupt to start the TX Transfer.      */
         HWREG(UartContext.Base + UART_O_IM) |= UART_IM_TXIM;

         /* Start sending data to the Uart Transmit.                    */
         MAP_IntDisable(UartContext.IntBase);
         TxInterrupt();
         MAP_IntEnable(UartContext.IntBase);
      }

      ret_val = 0;
   }
   else
      ret_val = HCITR_ERROR_WRITING_TO_PORT;

   return(ret_val);
}

   /* The following function is called when an invalid start of packet  */
   /* byte is received.  This function can this handle this case as     */
   /* needed.  The first parameter is the Transport ID that was         */
   /* from a successfull call to HCITR_COMWrite and the second          */
   /* parameter is the invalid start of packet byte that was received.  */
void BTPSAPI HCITR_InvalidStart_Callback(unsigned int HCITransportID, unsigned char Data)
{
   /* * NOTE * eHCILL characters may be processed here.                 */
}
