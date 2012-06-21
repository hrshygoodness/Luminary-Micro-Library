 //*****************************************************************************
 //
 // bt_spp.c - Main application for Stellaris Bluetooth SPP Demo
 //            application.
 //
 // Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
 // Software License Agreement
 // 
 // Texas Instruments (TI) is supplying this software for use solely and
 // exclusively on TI's microcontroller products. The software is owned by
 // TI and/or its suppliers, and is protected under applicable copyright
 // laws. You may not combine this software with "viral" open-source
 // software in order to form a larger program.
 // 
 // THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
 // NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
 // NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 // A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
 // CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 // DAMAGES, FOR ANY REASON WHATSOEVER.
 // 
 // This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2560-BLUETOPIA Firmware Package.
 //
 //*****************************************************************************

 #include "inc/hw_types.h"
 #include "inc/hw_memmap.h"
 #include "inc/hw_ints.h"
 #include "inc/hw_gpio.h"
 #include "driverlib/gpio.h"
 #include "driverlib/interrupt.h"
 #include "driverlib/pin_map.h"
 #include "driverlib/rom.h"
 #include "driverlib/sysctl.h"
 #include "driverlib/timer.h"
 #include "driverlib/uart.h"
 #include "utils/ustdlib.h"

 #include "bluetooth.h"
 #include "graphics.h"

 #include "BTPSKRNL.h"

 //*****************************************************************************
 //
 //! \addtogroup example_list
 //! <h1>Bluetooth Serial Port Profile (bt_spp)</h1>
 //!
 //! This application provides a Bluetooth Serial Port Profile (SPP) interface
 //! that allows it to receive and draw accelerometer data that is sent from a
 //! remote SPP server.  The development board must be equipped with an EM2
 //! expansion board and a CC2560/PAN1323 Bluetooth radio transceiver module for
 //! this application to run correctly.  The CC2560/PAN1323XX module must be
 //! installed in the ``mod1'' connector (the connector nearest the oscillator)
 //! on the EM2 expansion board.
 //!
 //! The application uses the Bluetooth Serial Port Profile (SPP) to receive
 //! specially formatted X and Y accelerometer readings from a remote SPP
 //! server.  The Stellaris Bluetooth kit includes an eZ430-RF2560 Bluetooth
 //! evaluation tool for this purpose.  The bt_spp application running on the
 //! development board searches for and connects to the remote device with the
 //! name "Blue-MSP430Demo" (the eZ430) when you press the user button and then
 //! waits for data from this remote device.  Whenever accelerometer data is
 //! received, the data is drawn on the display.
 //!
 //! Assuming you have already loaded the example application into the flash
 //! memory of the development board, follow these steps to run the example
 //! program:
 //!
 //! - Turn on the development board.  The display should show: ``Bluetooth
 //! BlueMSP430 Demo.  Press Button to search for devices.''
 //! - Press the user button on the development board to search for devices.
 //! - The display should indicate ``Searching for devices...'' and
 //! ``Device Found: [BD_ADDR]''.
 //! - Power on the eZ430-RF2560 (connected to the battery board) by attaching
 //! a jumper to JP1.
 //! - The red power LED should turn on and the blue LED should flash
 //! periodically to indicate the eZ430 device is discoverable.
 //! - Once connected, the development board display should indicate
 //! ``Device Connected:'' followed by the Bluetooth address of the device.
 //! - Move the eZ430 board around to ``draw'' on the development board
 //! accelerometer display.
 //!
 //! PLEASE NOTE: Sometimes when ``drawing'' with the eZ430RF2560 board, a
 //! ``jump'' in the data will appear on the screen.  This is normal and does
 //! not mean there is anything wrong with the kit.  This happens because
 //! there can be occasional discontinuities in the raw accelerometer data from
 //! the eZ430-RD2560 board.
 //!
 //! Refer to the README-First document that was included on the CD that came
 //! with the Stellaris Bluetooth kit for diagrams and detailed instructions.
 //!
 //*****************************************************************************

 //*****************************************************************************
 //
 // This MACRO can be used to display a debug message
 //
 //*****************************************************************************
 #define Display(_Message)       DBG_MSG(DBG_ZONE_DEVELOPMENT, _Message)

 //*****************************************************************************
 //
 // Defines the GPIO ports and pins used for LEDs and buttons.
 //
 //*****************************************************************************
 #define LED_PORT            GPIO_PORTF_BASE
 #define LED_PIN             GPIO_PIN_3
 #define USER_BUTTON_PORT    GPIO_PORTJ_BASE
 #define USER_BUTTON_PIN     GPIO_PIN_7

 //*****************************************************************************
 //
 // The following defines Count Values that are used to time events.
 //
 //*****************************************************************************
 #define HUNDREDTH_SEC_COUNT     10
 #define ONE_SEC_COUNT           100
 #define TOGGLE_COUNT            5
 #define MAX_COUNT               15

 //*****************************************************************************
 //
 // An array to hold a string for the board address.
 //
 //*****************************************************************************
 static char g_cBoardAddress[(SIZE_OF_BD_ADDR << 1) + 2 + 1];

 //*****************************************************************************
 //
 // Tick count of tick timer used by Bluetooth stack.
 //
 //*****************************************************************************
 static volatile unsigned long g_ulTickCount;

 //*****************************************************************************
 //
 // Flag to indicate device discovery.
 //
 //*****************************************************************************
 static tBoolean g_bDeviceDiscoveryActive = false;

 //*****************************************************************************
 //
 // Flag to indicate device connection.
 //
 //*****************************************************************************
 static tBoolean g_bConnected = false;

 //*****************************************************************************
 //
 // Count of successive times that user button is pressed.
 //
 //*****************************************************************************
 static int g_iPressCount = 0;

 //*****************************************************************************
 //
 // Counts timer ticks for timing intervals.
 //
 //*****************************************************************************
 static int g_iTick = ONE_SEC_COUNT;

 #ifdef DEBUG_ENABLED
 //*****************************************************************************
 //
 // The following defines the mapping strings for the Bluetooth Version.
 //
 //*****************************************************************************
 static char *g_pcHCIVersionStrings[] =
 {
     "1.0b",
     "1.1",
     "1.2",
     "2.0",
     "2.1",
     "3.0",
     "4.0",
     "Unknown (greater 4.0)"
 } ;

 #define NUM_SUPPORTED_HCI_VERSIONS                                            \
         (sizeof(g_pcHCIVersionStrings)/sizeof(char *) - 1)

 #endif

 //*****************************************************************************
 //
 // The error routine that is called if the driver library encounters an error.
 //
 //*****************************************************************************
 #ifdef DEBUG
 void
 __error__(char *pcFilename, unsigned long ulLine)
 {
 }
 #endif

 //*****************************************************************************
 //
 // The following function toggles the state of specified led.
 //
 //*****************************************************************************
 static void
 ToggleLED(unsigned long ulLEDPin)
 {
     ROM_GPIOPinWrite(LED_PORT, ulLEDPin, ~ROM_GPIOPinRead(LED_PORT, ulLEDPin));
 }

 //*****************************************************************************
 //
 // The following function is used to check the state of the User
 // Button.  The function returns TRUE if the button is depressed and
 // FALSE if it is released.
 //
 //*****************************************************************************
 static
 Boolean_t UserSwitchPressed(void)
 {
     //
     // Button GPIO reads as 0 when pressed so invert the logic sense when
     // returning the value.
     //
     return(!ROM_GPIOPinRead(USER_BUTTON_PORT, USER_BUTTON_PIN));
 }

 //*****************************************************************************
 //
 // Function to format the BD_ADDR as a string.
 //
 //*****************************************************************************
 static void
 BD_ADDRToStr(unsigned char *pucBoard_Address, char *pcBoardStr)
 {
     unsigned int uIdx;

     usprintf(pcBoardStr, "0x");
     for(uIdx = 0; uIdx < 6; uIdx++)
     {
         pcBoardStr += 2;
         usprintf(pcBoardStr, "%02X", pucBoard_Address[uIdx]);
     }
 }

 //*****************************************************************************
 //
 // The following function is called when various Bluetooth Events occur.  The
 // function passes a Callback Event Data Structure and a Callback Parameter.
 // The Callback parameter is a user definable value that was passed to the
 // InitializeBluetooth function.  For this application, this value is not used.
 //
 //*****************************************************************************
 static void
 BluetoothCallbackFunction(tCallbackEventData *pCallbackData,
                           void *pvCallbackParameter)
 {
     int iResult;
     char cStatus[48];
     unsigned char ucBuffer[32];
     unsigned char *pucBufPtr;
     short sX;
     short sY;
     short sRawX = 0;
     short sRawY = 0;

     //
     // Verify that the parameters pass in appear valid.
     //
     if(pCallbackData)
     {
         //
         // Process each Callback Event.
         //
         switch(pCallbackData->sEvent)
         {
             //
             // Process each Callback Event.
             //
             case cePinCodeRequest:
             {
                 Display(("cePinCodeRequest\r\n"));
                 PinCodeResponse(pCallbackData->ucRemoteDevice, 4,
                                 DEFAULT_PIN_CODE);
                 break;
             }

             //
             // Handle completion of authentication
             //
             case ceAuthenticationComplete:
             {
                 Display(("ceAuthenticationComplete\r\n"));
                 break;
             }

             //
             // Handle failure of authentication
             //
             case ceAuthenticationFailure:
             {
                 Display(("ceAuthenticationFailure\r\n"));
                 break;
             }

             //
             // Handle device found event
             //
             case ceDeviceFound:
             {
                 Display(("ceDeviceFound\r\n"));

                 //
                 // Get the device address and show it on the debug console
                 //
                 BD_ADDRToStr(pCallbackData->ucRemoteDevice, g_cBoardAddress);
                 Display(("BD_ADDR: %s\r\n", g_cBoardAddress));

                 //
                 // Update the address box on the display panel
                 //
                 usprintf(cStatus, "Device Found: %s.", g_cBoardAddress);
                 UpdateStatusBox(cStatus);
                 break;
             }

             //
             // Handle a retry event
             //
             case ceDeviceRetry:
             {
                 Display(("ceDeviceRetry\r\n"));

                 //
                 // Get the device address and show it on the debug console
                 //
                 BD_ADDRToStr(pCallbackData->ucRemoteDevice, g_cBoardAddress);
                 Display(("BD_ADDR: %s\r\n", g_cBoardAddress));

                 //
                 // Update the address box on the display panel
                 //
                 usprintf(cStatus, "Retrying to %s", g_cBoardAddress);
                 UpdateStatusBox(cStatus);

                 //
                 // Stop the service discovery operation.
                 //
                 if(g_bDeviceDiscoveryActive)
                 {
                     DeviceDiscovery(false);
                     g_bDeviceDiscoveryActive = false;
                 }
                 break;
             }

             //
             // Handle connection failure
             //
             case ceDeviceConnectionFailure:
             {
                 Display(("ceDeviceConnectionFailure\r\n"));

                 //
                 // Get the device address and show it on the debug console
                 //
                 BD_ADDRToStr(pCallbackData->ucRemoteDevice, g_cBoardAddress);
                 Display(("BD_ADDR: %s\r\n", g_cBoardAddress));

                 //
                 // Update the address box on the display panel
                 //
                 usprintf(cStatus, "Connect failed: %s.", g_cBoardAddress);
                 UpdateStatusBox(cStatus);

                 //
                 // Restart Device Discovery.
                 //
                 if(!g_bDeviceDiscoveryActive)
                 {
                     DeviceDiscovery(true);
                     g_bDeviceDiscoveryActive = true;
                 }
                 break;
             }

             //
             // Handle case of device connection
             //
             case ceDeviceConnected:
             {
                 Display(("ceDeviceConnected\r\n"));

                 //
                 // Get the device address and show it on the debug console
                 //
                 BD_ADDRToStr(pCallbackData->ucRemoteDevice, g_cBoardAddress);
                 Display(("BD_ADDR: %s\r\n", g_cBoardAddress));

                 //
                 // Turn off device discovery.
                 //
                 if(g_bDeviceDiscoveryActive)
                 {
                     DeviceDiscovery(false);
                     g_bDeviceDiscoveryActive = false;
                 }

                 //
                 // Flag that we are now g_bConnected.
                 //
                 g_bConnected = true;

                 //
                 // Update the address box on the display panel
                 //
                 usprintf(cStatus, "Device Connected: %s.", g_cBoardAddress);
                 UpdateStatusBox(cStatus);

                 //
                 // Show the accelerometer "drawing" screen on the display
                 //
                 SwitchToAccelScreen();
                 break;
             }

             //
             // Handle the device disconnection
             //
             case ceDeviceDisconnected:
             {
                 Display(("ceDeviceDisconnected\r\n"));

                 //
                 // Get the device address and show it on the debug console
                 //
                 BD_ADDRToStr(pCallbackData->ucRemoteDevice, g_cBoardAddress);
                 Display(("BD_ADDR: %s\r\n", g_cBoardAddress));

                 //
                 // Flag that we are now no longer g_bConnected.
                 //
                 g_bConnected = false;

                 //
                 // Update the address box on the display panel
                 //
                 usprintf(cStatus, "Device Disconnected: %s.", g_cBoardAddress);
                 UpdateStatusBox(cStatus);

                 //
                 // Change the display back to the main screen
                 //
                 SwitchToMainScreen();

                 //
                 // Show the user a message prompt to press user button
                 //
                 UpdateStatusBox("Press Button to search for devices.");
                 break;
             }

             //
             // Handle received data
             //
             case ceDataReceived:
             {
                 //
                 // Read the data received.
                 //
                 iResult = ReadData(sizeof(ucBuffer), ucBuffer);
                 if(iResult > 0)
                 {
                     // Process the 4 byte messages that we have received.
                     // * NOTE * The ReadData function cannot return more
                     //          bytes than are in the buffer thus we do not
                     //          need to check this in the while loop below.
                     pucBufPtr = ucBuffer;
                     while(iResult >= 4)
                     {
                         //
                         // The first two bytes are the X-axis encoded in 2
                         // bytes (followed by the Y-Axis).
                         //
                         sX = (pucBufPtr[1] << 8) | (pucBufPtr[0]);
                         sY = (pucBufPtr[3] << 8) | (pucBufPtr[2]);
                         pucBufPtr += 4;
                         iResult -= 4;

                         //
                         // Subtract the offset added by the MSP430.
                         //
                         sX = (sX - 2048) / 10;
                         sY = (sY - 2048) / 10;
                         sRawX = (sY<<1);
                         sRawY = -(sX<<1);

                         //
                         // Show the data on the debug console
                         //
                         Display(("X_Axis: %d.\r\n", sRawX));
                         Display(("Y_Axis: %d.\r\n", sRawY));

                         //
                         // Update the display panel to show the X-Y value.
                         //
                         ProcessAccelData(sRawX, sRawY);
                     }
                 }
                 break;
             }
         }
     }
 }

 //*****************************************************************************
 //
 // The following function is called from the Bluetooth kernel scheduler every
 // 10ms to update the LED and check the state of the button.
 //
 //*****************************************************************************
 static void BTPSAPI
 TenMSFunction(void *pvScheduleParameter)
 {
     //
     // If one second has elapsed, then toggle the LED
     //
     if(!(--g_iTick))
     {
         g_iTick = ONE_SEC_COUNT;
         ToggleLED(LED_PIN);
     }

     //
     // Check to see if the User Switch was pressed.
     //
     if(UserSwitchPressed())
     {
         //
         // Count the amount of time that the button has been pressed.
         //
         g_iPressCount++;
     }

     //
     // Else the user switch is not pressed
     //
     else
     {
         //
         // Check to see if the button was just released.
         //
         if(g_iPressCount)
         {
             Display(("Press Count %d\r\n", g_iPressCount));

             //
             // If not already connected to a device, then toggle the device
             // discovery state, and update the message on the display panel.
             //
             if(!g_bConnected)
             {
                 if(!DeviceDiscovery(!g_bDeviceDiscoveryActive))
                 {
                     g_bDeviceDiscoveryActive = !g_bDeviceDiscoveryActive;
                     if(g_bDeviceDiscoveryActive)
                     {
                         UpdateStatusBox("Searching for devices...");
                     }
                     else
                     {
                         UpdateStatusBox("Press Button to search for "
                                         "devices.");
                     }
                 }
             }
             g_iPressCount = 0;
         }
     }
 }

 //*****************************************************************************
 //
 // The following function is the function that is registered with the
 // BTPS abstraction layer to support the retrieval of the current
 // millisecond tick count. This function is registered with the
 // system by putting the value of this function in the
 // GetTickCountCallback member of the BTPS_Initialization_t structure
 // (and passing this structure to the BTPS_Init() function.
 //
 //*****************************************************************************
 static unsigned long
 GetTickCountCallback(void)
 {
     //
     // Simply return the current Tick Count.
     //
     return(g_ulTickCount);
 }

 #ifdef DEBUG_ENABLED

 //*****************************************************************************
 //
 // Function that is registered with the Bluetooth system (via call to
 // BTPS_Init()) for debugging.  This function will be called back for
 // each character that is to be output to the debug terminal.
 //
 //*****************************************************************************
 static void BTPSAPI
 MessageOutputCallback(char cDebugCharacter)
 {
     //
     // Simply output the debug character.
     //
     UARTCharPut(UART0_BASE, cDebugCharacter);
 }

 #endif

 //*****************************************************************************
 //
 // The following is the Main Application Thread.  It will Initialize
 // the Bluetooth Stack and all used profiles.
 //
 //*****************************************************************************
 static void
 MainApp(void *ThreadParameter)
 {
     int iRetVal;
     tDeviceInfo sDeviceInfo;
     BTPS_Initialization_t sBTPSInitialization;

     //
     // Specify the function that will be responsible for querying the
     // current millisecond Tick Count.
     // * NOTE * This function *MUST* be specified
     //
     sBTPSInitialization.GetTickCountCallback  = GetTickCountCallback;
     sBTPSInitialization.MessageOutputCallback = NULL;

     //
     // Set the callback function the stack can use for printing to the
     // console.
     //
#ifdef DEBUG_ENABLED
    sBTPSInitialization.MessageOutputCallback = MessageOutputCallback;
#else
    sBTPSInitialization.MessageOutputCallback = NULL;
#endif

    //
    // Initialize the Bluetooth stack, using no callback parameters (NULL).
    //
    iRetVal = InitializeBluetooth(BluetoothCallbackFunction, NULL,
                                  &sBTPSInitialization);

    //
    // Initialize the Graphics Module.
    //
    InitializeGraphics();

    //
    // Proceed with application if there was no Bluetooth init error.
    //
    if(!iRetVal)
    {
        //
        // Make the device Connectable and Discoverable and enabled
        // Secured Simple Pairing.
        //
        SetLocalDeviceMode(CONNECTABLE_MODE | DISCOVERABLE_MODE |
                            PAIRABLE_SSP_MODE);

        //
        // Get information about our local device.
        //
        iRetVal = GetLocalDeviceInformation(&sDeviceInfo);
        if(!iRetVal)
        {
            //
            // Format the board address into a string, and display it on
            // the console.
            //
            BD_ADDRToStr(sDeviceInfo.ucBDAddr, g_cBoardAddress);
            Display(("Local BD_ADDR: %s\r\n", g_cBoardAddress));

            //
            // Display additional info about the device to the console
            //
            Display(("HCI Version  : %s\r\n",
                     g_pcHCIVersionStrings[sDeviceInfo.ucHCIVersion]));
            Display(("Connectable  : %s\r\n",
                     ((sDeviceInfo.sMode & CONNECTABLE_MODE) ? "Yes" : "No")));
            Display(("Discoverable : %s\r\n",
                     ((sDeviceInfo.sMode & DISCOVERABLE_MODE) ? "Yes" : "No")));
            if(sDeviceInfo.sMode & (PAIRABLE_NON_SSP_MODE | PAIRABLE_SSP_MODE))
            {
                Display(("Pairable     : Yes\r\n"));
                Display(("SSP Enabled  : %s\r\n",
                        ((sDeviceInfo.sMode & PAIRABLE_SSP_MODE) ?
                        "Yes" : "No")));
            }
            else
            {
                Display(("Pairable     : No\r\n"));
            }
        }

        //
        // Prompt user with a message on the display panel
        //
        UpdateStatusBox("Press Button to search for devices.");

        //
        // Add function to the Bluetooth stack scheduler that is called back
        // every 10 ms to do some work.
        //
        BTPS_AddFunctionToScheduler(TenMSFunction, NULL, HUNDREDTH_SEC_COUNT);

        //
        // Show message on debug console to indicate starting the bluetooth
        // kernel
        //
        Display(("Execute Scheduler\r\n"));

        //
        // Enter forever loop to run the Bluetooth stack and keep the display
        // panel updated
        //
        while(1)
        {
            //
            // Run the bluetooth stack.
            //
            BTPS_ProcessScheduler();

            //
            // Update the display panel.
            //
            ProcessGraphics();
        }
    }

    //
    // There was an error initializing Bluetooth
    //
    else
    {
        //
        // Print an error message to the console and show a message on
        // the screen
        //
        Display(("Bluetooth Failed to initialize:  Error %d\r\n", iRetVal));
        UpdateStatusBox("Failed to Initialize Bluetooth.");

        //
        // Enter a forever loop.  Continue to update the screen, and rapidly
        // blink the LED as an indication of the error state.
        //
        while(1)
        {
            ProcessGraphics();
            BTPS_Delay(500);
            ToggleLED(LED_PIN);
        }
    }
}

//*****************************************************************************
//
// The following function is used to configure the hardware platform for the
// intended use.
//
//*****************************************************************************
static void
ConfigureHardware(void)
{
    //
    // Set the system clock for 50 MHz.
    //
    ROM_SysCtlClockSet((SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                        SYSCTL_XTAL_16MHZ));

    //
    // Enable all the GPIO ports that are used for peripherals
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    //
    // Configure the pin functions for each GPIO port
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);
    GPIOPinConfigure(GPIO_PA6_USB0EPEN);
    GPIOPinConfigure(GPIO_PA7_USB0PFLT);

    GPIOPinConfigure(GPIO_PB2_I2C0SCL);
    GPIOPinConfigure(GPIO_PB3_I2C0SDA);
    GPIOPinConfigure(GPIO_PB6_I2S0TXSCK);
    GPIOPinConfigure(GPIO_PB7_NMI);

    GPIOPinConfigure(GPIO_PC6_U1RX);
    GPIOPinConfigure(GPIO_PC7_U1TX);

    GPIOPinConfigure(GPIO_PD0_I2S0RXSCK);
    GPIOPinConfigure(GPIO_PD1_I2S0RXWS);
    GPIOPinConfigure(GPIO_PD4_I2S0RXSD);
    GPIOPinConfigure(GPIO_PD5_I2S0RXMCLK);

    GPIOPinConfigure(GPIO_PE1_SSI1FSS);
    GPIOPinConfigure(GPIO_PE4_I2S0TXWS);
    GPIOPinConfigure(GPIO_PE5_I2S0TXSD);

    GPIOPinConfigure(GPIO_PF1_I2S0TXMCLK);
    GPIOPinConfigure(GPIO_PF2_LED1);
    GPIOPinConfigure(GPIO_PF3_LED0);
    GPIOPinConfigure(GPIO_PF4_SSI1RX);
    GPIOPinConfigure(GPIO_PF5_SSI1TX);

    GPIOPinConfigure(GPIO_PH4_SSI1CLK);

    GPIOPinConfigure(GPIO_PJ0_I2C1SCL);
    GPIOPinConfigure(GPIO_PJ1_I2C1SDA);
    GPIOPinConfigure(GPIO_PJ3_U1CTS);
    GPIOPinConfigure(GPIO_PJ6_U1RTS);

    //
    // Set up the GPIO port and pin used for the LED.
    //
    ROM_GPIOPinTypeGPIOOutput(LED_PORT, LED_PIN);
    ROM_GPIOPinWrite(LED_PORT, LED_PIN, 0);

    //
    // Set up the GPIO port and pin used for the user push button.
    //
    ROM_GPIOPinTypeGPIOInput(USER_BUTTON_PORT, USER_BUTTON_PIN);

    //
    // Configure the Shutdown Pin.
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);
    ROM_GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0);

    //
    // Set the current Output debug port (if debugging enabled).
    //
#ifdef DEBUG_ENABLED
    //
    // Configure UART 0 to be used as the debug console port.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, (GPIO_PIN_0 | GPIO_PIN_1));
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));
#endif

    //
    // Setup a 1 ms Timer to implement Tick Count required for Bluetooth
    // Stack.
    //
    g_ulTickCount = 0;
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    ROM_TimerDisable(TIMER0_BASE, TIMER_A);
    ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    ROM_TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0);

    //
    // Configure timer for 1 mS tick rate
    //
    ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet() / 1000);

    //
    // Enable timer interrupts for the 1ms timer
    //
    ROM_IntEnable(INT_TIMER0A);
    ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    ROM_TimerEnable(TIMER0_BASE, TIMER_A);

    //
    // Turn on all interrupts in the system.
    //
    ROM_IntMasterEnable();
}

//*****************************************************************************
//
// Timer Interrupt handler that is registered to process Timer Tick Interrupt
// used to keep current Tick Count required for the Bluetooth Stack.
//
//*****************************************************************************
void
TimerTick(void)
{
    //
    // Clear the interrupt and update the tick count.
    //
    ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    g_ulTickCount++;
}

//*****************************************************************************
//
// The following is the Main application entry point.  This function will
// configure the hardware and initialize the OS Abstraction layer, create
// the Main application thread and start the scheduler.
//
//*****************************************************************************
int main(void)
{
    //
    // Configure the hardware for its intended use.
    //
    ConfigureHardware();

    //
    // Call the application main loop (above).  This function will not return.
    //
    MainApp(NULL);

    return(0);
}
