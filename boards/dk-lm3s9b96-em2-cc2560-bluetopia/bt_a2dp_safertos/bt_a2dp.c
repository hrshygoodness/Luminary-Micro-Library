//*****************************************************************************
//
// bt_a2dp.c - Main application for Stellaris Bluetooth A2DP Demo
//             application.
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
#include "inc/hw_nvic.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "utils/ustdlib.h"

#include "SafeRTOS/SafeRTOS_API.h"

#include "bluetooth.h"
#include "dac32sound.h"
#include "graphics.h"

#include "BTPSKRNL.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Advanced Audio Distribution Profile (bt_a2dp_safertos)</h1>
//!
//! This application provides a Bluetooth A2DP streaming endpoint that is
//! capable of receiving audio data from Bluetooth-enabled A2DP sources and
//! playing the audio data out of the headset and lineout ports.  The
//! development kit must be equipped with an EM2 expansion board and a
//! CC2560/PAN1323 Bluetooth radio transceiver module for this application to
//! run correctly.  The CC2560/PAN1323 module must be installed in the ``mod1''
//! connector (the connector nearest the oscillator) on the EM2 expansion
//! board.
//!
//! The A2DP demo application uses the Bluetooth A2DP Profile to manage the
//! audio streaming connection.  The application creates and advertises
//! support for an A2DP audio sink endpoint.  The endpoint can be discovered
//! by Bluetooth devices that are capable of supporting the A2DP audio source
//! role.  Devices supporting the audio source role can connect to the sink
//! endpoint, configure the playback parameters, and stream audio data to the
//! audio sink for playback.  The A2DP sink provides support for the SBC codec.
//!
//! The A2DP demo supports the following three Bluetooth pairing modes:
//!
//! - No pairing
//! - Legacy pairing
//! - Secure simple pairing
//!
//! The application supports up to five persistent link keys.  The oldest link
//! keys are purged to make room for a newer link key once five have been
//! saved.
//!
//! When the application is running, the LED will toggle periodically.
//! Assuming you have already loaded the example program into the development
//! board, follow these steps to run the example program:
//!
//! - Attach the headphone output to any standard headphone or attach the line
//! output to an external amplifier.
//! - Turn on the development board.  The display should show ``Bluetooth A2DP
//! Demo Waiting for connection ...''.
//! - Using any Bluetooth device capable of A2DP source, search for the
//! development board.  Bluetooth inquiries will display the device friendly
//! name of ``A2DP Demo''.  If the source device requests a passkey, use
//! ``0000''.
//! - After a successful connection, the development board should change the
//! display to ``Connected ... Paused''.
//! - Start the Audio on the A2DP source device and you should hear audio (via
//! the headphone or attached speaker).
//!
//! During audio streaming, press the user button to adjust the headphone
//! port's audio volume.  The volume is set to 90% maximum volume at
//! initialization and decreases by 10% each time the user button is pressed.
//! The volume wraps to 100% once 0% is passed.
//
//*****************************************************************************

//*****************************************************************************
//
// The following defines the prototype of the Display function.  This is used
// to send debug messages out the Debug Port.
// * NOTE * This requires the pre-processor symbol DEBUG_ENABLED to be
// defined.
//
//*****************************************************************************
#define Display(_x) DBG_MSG(DBG_ZONE_DEVELOPMENT, _x)

//*****************************************************************************
//
// Defines the GPIO ports and pins used for LEDs and buttons.
//
//*****************************************************************************
#define LED_PORT GPIO_PORTF_BASE
#define LED_PIN GPIO_PIN_3
#define USER_BUTTON_PORT GPIO_PORTJ_BASE
#define USER_BUTTON_PIN GPIO_PIN_7

//*****************************************************************************
//
// Initial size of the system stack in bytes.  This will be passed to SafeRTOS
// when it is initialized and must match the amount of stack space that is
// allocated in the startup code file.
//
//*****************************************************************************
#define SYSTEM_STACK_SIZE (0x0800)

//*****************************************************************************
//
// The following defines the stack sizes for any tasks.
//
//*****************************************************************************
#define MAIN_APP_STACK_SIZE (configMINIMAL_STACK_SIZE + 1024)
#define IDLE_TASK_STACK_SIZE (configMINIMAL_STACK_SIZE + 512)

//*****************************************************************************
//
// Denotes the priority of the main application thread.
//
//*****************************************************************************
#define DEFAULT_THREAD_PRIORITY (3)

//*****************************************************************************
//
// The following defines Count Values that are used to time events.
//
//*****************************************************************************
#define TENTH_SEC_COUNT 100
#define ONE_SEC_COUNT 10
#define TOGGLE_COUNT 5
#define MAX_COUNT 15

//*****************************************************************************
//
// An array to hold a string for the board address.
//
//*****************************************************************************
static char g_cBoardAddress[(SIZE_OF_BD_ADDR << 1) + 2 + 1];

//*****************************************************************************
//
// Flag to indicate audio connection status.
//
//*****************************************************************************
static tBoolean g_bAudioConnected;

//*****************************************************************************
//
// Flag to indicate remote control connection status.
//
//*****************************************************************************
static tBoolean g_bRemoteConnected;

//*****************************************************************************
//
// Flag to indicate audio stream status.
//
//*****************************************************************************
static tBoolean g_bStreamStarted;

//*****************************************************************************
//
// Arrays used as stack space for each task.
//
//*****************************************************************************
static unsigned long g_ulMainAppThreadStack[(MAIN_APP_STACK_SIZE) /
                                            sizeof(unsigned long) + 1];
static unsigned long g_ulIdleTaskStack[(IDLE_TASK_STACK_SIZE) /
                                       sizeof(unsigned long) + 1];

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
// The following function toggles the state of specified LED.
//
//*****************************************************************************
static void
ToggleLED(unsigned long ulLEDPin)
{
    ROM_GPIOPinWrite(LED_PORT, ulLEDPin, ~ROM_GPIOPinRead(LED_PORT, ulLEDPin));
}

//*****************************************************************************
//
// The following function is used to check the state of the User Button.  The
// function returns TRUE if the button is depressed and FALSE if it is
// released.
//
//*****************************************************************************
static tBoolean
UserSwitchPressed(void)
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
                          void *pCallbackParameter)
{
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
            // Handle PIN code request
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
            // Handle opening of endpoint.  Notify user of connection.
            //
            case ceAudioEndpointOpen:
            {
                Display(("ceAudioEndpointOpen\r\n"));

                //
                // Flag that we are now connected.
                //
                g_bAudioConnected = true;

                //
                // Flag that the Audio is not currently playing.
                //
                g_bStreamStarted  = false;

                //
                // Set the Audio State to indicate that we need to prepare to
                // receive audio data.
                //
                UpdateStatusBox("Connected... Paused");
                break;
            }

            //
            // Handle closing of endpoint.  Notify user there is no more
            // connection.
            //
            case ceAudioEndpointClose:
            {
                Display(("ceAudioEndpointClose\r\n"));

                //
                // Flag that we are no longer connected.
                //
                g_bAudioConnected = false;

                //
                // Flag that the Audio is NOT currently playing.
                //
                g_bStreamStarted  = false;

                //
                // Set the audio State back to Stopping.  This will allow any
                // queued audio data to be consumed.
                //
                UpdateStatusBox("Waiting for Connection...");
                break;
            }

            //
            // Handle start of audio stream. Notify user that a stream is
            // playing.
            //
            case ceAudioStreamStart:
            {
                Display(("ceAudioStreamStart\r\n"));

                //
                // Flag that the Audio is currently playing.
                //
                g_bStreamStarted  = true;

                UpdateStatusBox("Connected... Playing");
                break;
            }

            //
            // Handle suspension of audio stream.  Notify user that the stream
            // is paused.
            //
            case ceAudioStreamSuspend:
            {
                Display(("ceAudioStreamSuspend\r\n"));

                //
                // Flag that the Audio is NOT currently playing.
                //
                g_bStreamStarted  = false;

                UpdateStatusBox("Connected... Paused");
                break;
            }

            //
            // Handle opening of control connection.
            //
            case ceRemoteControlConnectionOpen:
            {
                Display(("ceRemoteControlConnectionOpen\r\n"));

                //
                // Flag that the Remote is connected.
                //
                g_bRemoteConnected = true;
                break;
            }

            //
            // Handle closing of control connection.
            //
            case ceRemoteControlConnectionClose:
            {
                Display(("ceRemoteControlConnectionClose\r\n"));

                //
                // Flag that the Remote is no longer connected.
                //
                g_bRemoteConnected = false;
                break;
            }
        }
    }
}

#ifdef DEBUG_ENABLED

//*****************************************************************************
//
// Function that is registered with the Bluetooth system (via call to
// BTPS_Init()) for debugging.  This function will be called back for each
// character that is to be output to the debug terminal.
//
//*****************************************************************************
static void BTPSAPI
MessageOutputCallback(char cDebugCharacter)
{
    //
    // Simply output the debug character.
    //
    ROM_UARTCharPut(UART0_BASE, cDebugCharacter);
}

#endif

//*****************************************************************************
//
// The following is the callback function that is registered with the
// graphics module (during the InitializeGraphics() call).  This function
// is called by the Graphics module when a button press occurs.
//
//*****************************************************************************
static void
ButtonPressCallback(unsigned int ButtonPress)
{
    //
    // Only process if the Audio Endpoint AND the Remote is
    // connected.
    //
    if(g_bAudioConnected && g_bRemoteConnected)
    {
        //
        // Both connected, process the Button press.
        //
        switch(ButtonPress)
        {
            case BUTTON_PRESS_PLAY:
            {
                Display(("Play Pressed\r\n"));

                //
                // Determine if we are currently playing or are
                // paused.
                //
                if(!g_bStreamStarted)
                {
                    if(!SendRemoteControlCommand(rcPlay))
                    {
                       UpdateStatusBox("Connected... Playing");

                       g_bStreamStarted = true;
                    }
                }
                break;
            }
            case BUTTON_PRESS_PAUSE:
            {
                Display(("Pause Pressed\r\n"));

                //
                // Determine if we are currently playing or are
                // paused.
                //
                if(g_bStreamStarted)
                {
                    if(!SendRemoteControlCommand(rcPause))
                    {
                       UpdateStatusBox("Connected... Playing");

                       g_bStreamStarted = false;
                    }
                }
                break;
            }
            case BUTTON_PRESS_NEXT:
            {
                Display(("Next Pressed\r\n"));

                SendRemoteControlCommand(rcNext);
                break;
            }
            case BUTTON_PRESS_BACK:
            {
                Display(("Back Pressed\r\n"));

                SendRemoteControlCommand(rcBack);
                break;
            }
            default:
            {
                //
                // Unknown/Unhandled button press.
                //
                break;
            }
        }
    }
}

//*****************************************************************************
//
// The following is the Main Application Thread.  It will Initialize the
// Bluetooth Stack and all used profiles.
//
//*****************************************************************************
static void
MainApp(void *pThreadParameter)
{
    int iPressCount;
    int iTick;
    int iVolume;
    int iRetVal;
    tDeviceInfo sDeviceInfo;
    BTPS_Initialization_t sBTPSInitialization;

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
    InitializeGraphics(ButtonPressCallback);

    //
    // Proceed with application if there was no Bluetooth init error.
    //
    if(!iRetVal)
    {
        //
        // Make the device Connectable and Discoverable and enabled Secured
        // Simple Pairing.
        //
        SetLocalDeviceMode(CONNECTABLE_MODE  | DISCOVERABLE_MODE |
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

            //
            // Show message to user on the screen
            //
            UpdateStatusBox("Waiting for Connection...");

            //
            // Bluetooth should be running now.  Enter a forever loop to run
            // the user interface on the board screen display and process
            // button presses.
            //
            iTick = ONE_SEC_COUNT;
            iVolume = DEFAULT_POWERUP_VOLUME;
            iPressCount = 0;
            while(1)
            {
                //
                // Update the screen.
                //
                ProcessGraphics();

                //
                // Wait 1/10 second.
                //
                BTPS_Delay(TENTH_SEC_COUNT);

                //
                // If one second has elapsed, toggle the LED
                //
                if(!(--iTick))
                {
                   iTick = ONE_SEC_COUNT;
                   ToggleLED(LED_PIN);
                }

                //
                // Check to see if the User Switch was pressed.
                //
                if(UserSwitchPressed())
                {
                   //
                   // Count the amount of time that the button has been
                   // pressed.
                   //
                   iPressCount++;
                }

                //
                // Else the user switch is not pressed
                //
                else
                {
                    //
                    // If the button was just released, then adjust the volume.
                    // Decrease the volume by 10% for each button press.  At
                    // zero, then reset it to 100%.
                    //
                    if(iPressCount)
                    {
                        iVolume = (iVolume == 0) ? 100 : iVolume - 10;

                        //
                        // Set the new volume, and display a message on the
                        // console
                        //
                        SoundVolumeSet(iVolume);
                        Display(("Press Count %d Volume %d\r\n", iPressCount,
                                iVolume));
                        iPressCount = 0;
                   }
                }
            }
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
// The following is the function that is registered with SafeRTOS to be
// called back when an error occurs.
//
//*****************************************************************************
static void
ApplicationErrorHook(xTaskHandle xCurrentTask,
                                 signed portCHAR *pcErrorString,
                                 portBASE_TYPE ErrorCode)
{
   while(1);
}

//*****************************************************************************
//
// The following is the function that is registered with SafeRTOS to be
// called back when a Task is deleted.
//
//*****************************************************************************
static void
ApplicationTaskDeleteHook(xTaskHandle DeletedTask)
{
}

//*****************************************************************************
//
// The following is the function that is registered with SafeRTOS to be
// called the SafeRTOS scheduler is Idle.
//
//*****************************************************************************
static void
ApplicationIdleHook(void)
{
    //
    // Simply call into the Bluetooth sub-system Application Idle Hook.
    //
    BTPS_ApplicationIdleHook();
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
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL  | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

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

#ifdef DEBUG_ENABLED
    //
    // Configure UART 0 to be used as the debug console port.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, (GPIO_PIN_0 | GPIO_PIN_1));
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8   | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));
#endif

    //
    // Turn on interrupts in the system.
    //
    ROM_IntMasterEnable();
}

//*****************************************************************************
//
// The following is the Main application entry point.  This function
// will configure the hardware and initialize the OS Abstraction layer,
// create the Main application thread and start the Safe RTOS Scheduler.
//
//*****************************************************************************
int
main(void)
{
    xTaskHandle MainTask;
    xPORT_INIT_PARAMETERS InitParams;

    //
    // Configure the hardware.
    //
    ConfigureHardware();

    //
    // Initialize the Safe RTOS scheduler.
    //
    InitParams.ulCPUClockHz = SysCtlClockGet();
    InitParams.ulTickRateHz = (1000 / portTICK_RATE_MS);
    InitParams.pxTaskDeleteHook = ApplicationTaskDeleteHook;
    InitParams.pxErrorHook = ApplicationErrorHook;
    InitParams.pxIdleHook = ApplicationIdleHook;
    InitParams.pulSystemStackLocation = *(unsigned long **)0;
    InitParams.ulSystemStackSizeBytes = SYSTEM_STACK_SIZE;
    InitParams.pulVectorTableBase = (unsigned portLONG *)HWREG(NVIC_VTABLE);

    //
    // Send the initialization parameters to the scheduler.
    //
    vTaskInitializeScheduler((signed char *)g_ulIdleTaskStack,
                             sizeof(g_ulIdleTaskStack), 0, &InitParams);

    //
    // Create the application main task
    //
    xTaskCreate(MainApp, NULL, (signed char *)g_ulMainAppThreadStack,
                sizeof(g_ulMainAppThreadStack), NULL, DEFAULT_THREAD_PRIORITY,
                &MainTask);

    //
    // Start the Task Scheduler.  This function will not return.
    //
    xTaskStartScheduler(pdTRUE);

    return(0);
}

