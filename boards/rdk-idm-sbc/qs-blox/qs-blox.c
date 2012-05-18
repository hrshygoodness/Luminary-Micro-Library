//*****************************************************************************
//
// qs-blox.c - A simple game of falling blocks.
//
// Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/flash.h"
#include "driverlib/timer.h"
#include "driverlib/systick.h"
#include "driverlib/epi.h"
#include "driverlib/rom.h"
#include "driverlib/udma.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "grlib/imgbutton.h"
#include "grlib/pushbutton.h"
#include "grlib/container.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "drivers/touch.h"
#include "drivers/jpgwidget.h"
#include "drivers/sdram.h"
#include "drivers/sound.h"
#include "drivers/wav.h"
#include "drivers/set_pinout.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/swupdate.h"
#include "utils/ustdlib.h"
#include "third_party/blox/blox.h"
#include "httpserver_raw/httpd.h"
#include "blox_screen.h"
#include "blox_web.h"
#include "images.h"
#include "sound_effects.h"
#include "usb_keyboard.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>Blox Game (qs-blox)</h1>
//!
//! Blox is a version of a well-known game in which you attempt to stack
//! falling colored blocks without leaving gaps. Blocks may be moved up or
//! down, rotated and dropped using onscreen widgets or an attached USB
//! keyboard.  When any row of the game area is filled with coloured squares,
//! that row is removed providing more space to drop later blocks into.  The
//! game ends when there is insufficient space on the playing area for a new
//! block to be placed on the board.  Your score accumulates over time with the
//! number of points awarded for dropping each block dependent upon the height
//! from which the block was dropped.
//!
//! When playing with the USB keyboard, the controls are as follow:
//!
//! <dl>
//! <dt>Up or Right arrow</dt>
//! <dd>Move the current block towards the top of the game area.</dd>
//! <dt>Down or Left arrow</dt>
//! <dd>Move the current block towards the bottom of the game area.</dd>
//! <dt>Space</dt>
//! <dd>Drop the block.</dd>
//! <dt>R</dt>
//! <dd>Rotate the block 90 degrees clockwise.</dd>
//! <dt>P</dt>
//! <dd>Pause or unpause the game.</dd>
//! </dl>
//!
//! The game also offers a small web site which displays the current status
//! and allows the difficulty level to be set.  This site employs AJAX to
//! request an XML file containing status information from the IDM and uses
//! this to update the score, hiscore and state information once every 2.5
//! seconds.
//!
//! This application supports remote software update over Ethernet using the
//! LM Flash Programmer application.  A firmware update is initiated using the
//! remote update request ``magic packet'' from LM Flash Programmer.
//
//*****************************************************************************

//*****************************************************************************
//
// The DMA control structure table.  This is required by the sound driver.
//
//*****************************************************************************
#ifdef ewarm
#pragma data_alignment=1024
tDMAControlTable sDMAControlTable[64];
#elif defined(ccs)
#pragma DATA_ALIGN(sDMAControlTable, 1024)
tDMAControlTable sDMAControlTable[64];
#else
tDMAControlTable sDMAControlTable[64] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The current state of the game.
//
//*****************************************************************************
tGameState g_eGameState = BLOX_WAITING;

//*****************************************************************************
//
// The number of SysTick interrupts we want per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 1000

//*****************************************************************************
//
// The number of lwIP stack ticks we want per second.
//
//*****************************************************************************
#define LWIP_TICKS_PER_SECOND 100
#define LWIP_DIVIDER (TICKS_PER_SECOND / LWIP_TICKS_PER_SECOND)

//*****************************************************************************
//
// A counter used to divide the system tick rate down to the lwIP tick rate.
//
//*****************************************************************************
unsigned long g_ulLWIPDivider = 0;

//*****************************************************************************
//
// The number of milliseconds that have passed since the system booted.
//
//*****************************************************************************
volatile unsigned long g_ulSysTickCount;

//*****************************************************************************
//
// User input flags used to control the game.
//
//*****************************************************************************
volatile unsigned long g_ulCommandFlags;

//*****************************************************************************
//
// The countdown value used just prior to starting a new game.
//
//*****************************************************************************
unsigned long g_ulCountdown;

//*****************************************************************************
//
// A flag used to indicate that an ethernet remote firmware update request
// has been recieved.
//
//*****************************************************************************
volatile tBoolean g_bFirmwareUpdate = false;

//*****************************************************************************
//
// Flag indicating whether or not we are currently playing a sound effect.
//
//*****************************************************************************
volatile tBoolean g_bSoundPlaying = false;

//*****************************************************************************
//
// Sound effect state information.
//
//*****************************************************************************
tWaveHeader g_sSoundEffectHeader;

//*****************************************************************************
//
// Forward reference to various widget structures.
//
//*****************************************************************************
extern tJPEGWidget g_sBackground;
extern tCanvasWidget g_sGameCanvas;
extern tCanvasWidget g_sStoppedCanvas;
extern tCanvasWidget g_sNextPiece;
extern tCanvasWidget g_sNextTitle;
extern tCanvasWidget g_sScore;
extern tCanvasWidget g_sScoreTitle;
extern tCanvasWidget g_sIPAddr;
extern tCanvasWidget g_sMACAddr;
extern tContainerWidget g_sGameBorder;
extern tImageButtonWidget g_sRotatePushBtn;
extern tImageButtonWidget g_sUpPushBtn;
extern tImageButtonWidget g_sDownPushBtn;
extern tImageButtonWidget g_sPausePushBtn;
extern tImageButtonWidget g_sDropPushBtn;
extern tPushButtonWidget g_sStartButton;

//*****************************************************************************
//
// Forward references to the button press handlers.
//
//*****************************************************************************
void OnDownButtonPress(tWidget *pWidget);
void OnUpButtonPress(tWidget *pWidget);
void OnPauseButtonPress(tWidget *pWidget);
void OnDropButtonPress(tWidget *pWidget);
void OnRotateButtonPress(tWidget *pWidget);
void OnStartButtonPress(tWidget *pWidget);

//*****************************************************************************
//
// The container drawing the border around the game area.
//
//*****************************************************************************
Container(g_sGameBorder, WIDGET_ROOT, 0, &g_sStoppedCanvas,
          &g_sKitronix320x240x16_SSD2119, GAME_AREA_LEFT - 1, GAME_AREA_TOP - 1,
          GAME_AREA_WIDTH + 2, GAME_AREA_HEIGHT + 2, CTR_STYLE_OUTLINE, 0,
          ClrWhite, 0, 0, 0);

//*****************************************************************************
//
// Workspace structure for the main JPEG image viewing widget.
//
//*****************************************************************************
tJPEGInst g_sMainJPEGInst;

//*****************************************************************************
//
// The widget forming the main image display area.
//
//*****************************************************************************
JPEGCanvas(g_sBackground, WIDGET_ROOT, &g_sGameBorder, &g_sUpPushBtn,
           &g_sKitronix320x240x16_SSD2119, 0, 0, 320, 240,
           JW_STYLE_LOCKED, 0, 0, 0, 0, 0, 0, 0, 0, 0, &g_sMainJPEGInst);

//*****************************************************************************
//
// The canvas widget into which we draw the next game piece.
//
//*****************************************************************************
Canvas(g_sNextPiece, &g_sGameCanvas, 0, 0,
       &g_sKitronix320x240x16_SSD2119, 20, 44, (2 * GAME_BLOCK_SIZE),
       (4 * GAME_BLOCK_SIZE), CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0,
       OnNextPiecePaint);

//*****************************************************************************
//
// The canvas widget acting as the main game playing area.
//
//*****************************************************************************
Canvas(g_sGameCanvas, &g_sGameBorder, 0, &g_sNextPiece,
       &g_sKitronix320x240x16_SSD2119, GAME_AREA_LEFT, GAME_AREA_TOP,
       GAME_AREA_WIDTH, GAME_AREA_HEIGHT,
       CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnGameAreaPaint);

//*****************************************************************************
//
// The button used to start a new game.
//
//*****************************************************************************
#define START_BTN_HEIGHT 24
#define START_BTN_WIDTH  80

RectangularButton(g_sStartButton, &g_sStoppedCanvas, 0, 0,
                  &g_sKitronix320x240x16_SSD2119,
                  GAME_AREA_LEFT + ((GAME_AREA_WIDTH - START_BTN_WIDTH) /2),
                  (GAME_AREA_TOP + GAME_AREA_HEIGHT) - (START_BTN_HEIGHT + 10),
                  START_BTN_WIDTH, START_BTN_HEIGHT,
                  (PB_STYLE_TEXT | PB_STYLE_FILL | PB_STYLE_OUTLINE |
                  PB_STYLE_RELEASE_NOTIFY), ClrDarkBlue, ClrBlue, ClrWhite,
                  ClrWhite, g_pFontCmss20, "Start", 0, 0, 0, 0,
                  OnStartButtonPress);

//*****************************************************************************
//
// The canvas widget covering the game area when the game is stopped.
//
//*****************************************************************************
Canvas(g_sStoppedCanvas, &g_sGameBorder, 0, &g_sStartButton,
       &g_sKitronix320x240x16_SSD2119, GAME_AREA_LEFT, GAME_AREA_TOP,
       GAME_AREA_WIDTH, GAME_AREA_HEIGHT,
       CANVAS_STYLE_APP_DRAWN, 0, 0, 0, 0, 0, 0, OnStopAreaPaint);

//*****************************************************************************
//
// The button used to move game pieces down.
//
//*****************************************************************************
ImageButton(g_sUpPushBtn, &g_sBackground, &g_sDownPushBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 271, 62, 38, 40,
            0, 0, 0, 0, 0, 0, g_pucBlueButton38x40Up, g_pucBlueButton38x40Down,
            g_pucUpKeyCap24x24, 2, 2, 0, 0, OnUpButtonPress);

//*****************************************************************************
//
// The button used to move game pieces down.
//
//*****************************************************************************
ImageButton(g_sDownPushBtn, &g_sBackground, &g_sRotatePushBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 271, 102, 38, 40,
            0, 0, 0, 0, 0, 0, g_pucBlueButton38x40Up, g_pucBlueButton38x40Down,
            g_pucDownKeyCap24x24, 2, 2, 0, 0, OnDownButtonPress);

//*****************************************************************************
//
// The button used to rotate game pieces.
//
//*****************************************************************************
ImageButton(g_sRotatePushBtn, &g_sBackground, &g_sDropPushBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 271, 142, 38, 40,
            0, 0, 0, 0, 0, 0, g_pucBlueButton38x40Up, g_pucBlueButton38x40Down,
            g_pucRotateKeyCap24x24, 2, 2, 0, 0, OnRotateButtonPress);

//*****************************************************************************
//
// The button used to drop game pieces.
//
//*****************************************************************************
ImageButton(g_sDropPushBtn, &g_sBackground, &g_sPausePushBtn, 0,
            &g_sKitronix320x240x16_SSD2119, 271, 182, 38, 40,
            0, 0, 0, 0, 0, 0, g_pucBlueButton38x40Up, g_pucBlueButton38x40Down,
            g_pucDropKeyCap24x24, 2, 2, 0, 0, OnDropButtonPress);

//*****************************************************************************
//
// The button used to pause the game.
//
//*****************************************************************************
ImageButton(g_sPausePushBtn, &g_sBackground, &g_sScore, 0,
            &g_sKitronix320x240x16_SSD2119, 271, 10, 38, 40,
            0, 0, 0, 0, 0, 0, g_pucBlueButton38x40Up, g_pucBlueButton38x40Down,
            g_pucPauseKeyCap24x24, 2, 2, 0, 0, OnPauseButtonPress);

//*****************************************************************************
//
// The current score.
//
//*****************************************************************************
char g_pcScore[MAX_SCORE_LEN] = "  0  ";

Canvas(g_sScore, &g_sBackground, &g_sScoreTitle, 0,
       &g_sKitronix320x240x16_SSD2119, 210, 62, 50, 20,
       (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
       CANVAS_STYLE_TEXT_HCENTER), BACKGROUND_COLOR, 0, SCORE_COLOR,
       g_pFontCmss20, g_pcScore, 0, 0);

Canvas(g_sScoreTitle, &g_sBackground, &g_sMACAddr, 0,
       &g_sKitronix320x240x16_SSD2119, 210, 40, 50, 20,
       (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_OPAQUE |
       CANVAS_STYLE_TEXT_HCENTER), BACKGROUND_COLOR, 0, SCORE_COLOR,
       g_pFontCmss20, "Score", 0, 0);


//*****************************************************************************
//
// The canvas widget used to display the board MAC address.
//
//*****************************************************************************
char g_pcMACAddr[24];
Canvas(g_sMACAddr, &g_sBackground, &g_sIPAddr, 0,
       &g_sKitronix320x240x16_SSD2119, (GAME_AREA_LEFT + (GAME_AREA_WIDTH / 2)),
       226, (GAME_AREA_WIDTH / 2), 10,
       (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_HCENTER), BACKGROUND_COLOR, 0,
       TEXT_COLOR, g_pFontFixed6x8, g_pcMACAddr, 0, 0);

//*****************************************************************************
//
// The canvas widget used to display the board IP address.
//
//*****************************************************************************
char g_pcIPAddr[24];
Canvas(g_sIPAddr, &g_sBackground, 0, 0,
       &g_sKitronix320x240x16_SSD2119, GAME_AREA_LEFT,
       226, (GAME_AREA_WIDTH / 2), 10,
       (CANVAS_STYLE_TEXT | CANVAS_STYLE_TEXT_HCENTER), BACKGROUND_COLOR, 0,
       TEXT_COLOR, g_pFontFixed6x8, g_pcIPAddr, 0, 0);

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
// This function is called by the software update module whenever a remote
// host requests to update the firmware on this board.  We set a flag that
// will cause the bootloader to be entered the next time the user enters a
// command on the console.
//
//*****************************************************************************
void SoftwareUpdateRequestCallback(void)
{
    g_bFirmwareUpdate = true;
}

//*****************************************************************************
//
// Widget handler for the "Down" button.
//
//*****************************************************************************
void OnDownButtonPress(tWidget *pWidget)
{
    g_ulCommandFlags |= BLOX_CMD_DOWN;
}

//*****************************************************************************
//
// Widget handler for the "Up" button.
//
//*****************************************************************************
void OnUpButtonPress(tWidget *pWidget)
{
    g_ulCommandFlags |= BLOX_CMD_UP;
}

//*****************************************************************************
//
// Widget handler for the "Pause" button.
//
//*****************************************************************************
void OnPauseButtonPress(tWidget *pWidget)
{
    g_ulCommandFlags |= BLOX_CMD_PAUSE;
}

//*****************************************************************************
//
// Widget handler for the "Drop" button.
//
//*****************************************************************************
void OnDropButtonPress(tWidget *pWidget)
{
    g_ulCommandFlags |= BLOX_CMD_DROP;
}

//*****************************************************************************
//
// Widget handler for the "Rotate" button.
//
//*****************************************************************************
void OnRotateButtonPress(tWidget *pWidget)
{
    g_ulCommandFlags |= BLOX_CMD_ROTATE;
}

//*****************************************************************************
//
// Widget handler for the "Start" button.
//
//*****************************************************************************
void OnStartButtonPress(tWidget *pWidget)
{
    //
    // Tell the main loop to switch into "Starting" mode and show the
    // countdown.
    //
    g_ulCommandFlags |= BLOX_CMD_START;
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.
//
//*****************************************************************************
void
SysTickIntHandler(void)
{
    //
    // Update our system timer counter.
    //
    g_ulSysTickCount++;

    //
    // Call the lwIP timer if necessary.
    //
    if(g_ulLWIPDivider == 0)
    {
        lwIPTimer(1000 / LWIP_TICKS_PER_SECOND);
        g_ulLWIPDivider = LWIP_DIVIDER;
    }

    //
    // Decrement the lwIP timer divider.
    //
    g_ulLWIPDivider--;
}

//*****************************************************************************
//
// Do any housekeeping necessary to keep the sound effects playing.
//
//*****************************************************************************
void
AudioSoundEffectProcess(void)
{
    tBoolean bComplete;

    //
    // Are we currently playing a sound effect?
    //
    if(g_bSoundPlaying)
    {
        //
        // Feed the wave file processor.
        //
        bComplete = WavePlayContinue(&g_sSoundEffectHeader);

        //
        // Did we finish playback of this sound effect?
        //
        if(bComplete)
        {
            //
            // Yes - remember that we are no longer playing anything.
            //
            g_bSoundPlaying = false;
        }
    }
}

//*****************************************************************************
//
// Play a new audio clip based on the game status flags passed.
//
//*****************************************************************************
void
PlayNewSoundEffect(unsigned long ulFlags)
{
    const unsigned char *pucNewSound;
    tWaveReturnCode eRetcode;

    //
    // Are we being told to do anything?
    //
    if(!(ulFlags & BLOX_STAT_MASK))
    {
        //
        // No - there is no new sound effect to play.
        //
        return;
    }

    //
    // Is the game over?
    //
    if(ulFlags & BLOX_STAT_END)
    {
        pucNewSound = g_pucGameOverSound;
    }
    else
    {
        //
        // Did a block reach the bottom of its drop?
        //
        if(ulFlags & BLOX_STAT_DROPPED)
        {
            pucNewSound = g_pucBumpSound;
        }
        else
        {
            //
            // Nothing special happened so just play the basic tick beep.
            //
            pucNewSound = g_pucBeepSound;
        }
    }

    //
    // Start playing the new sound.
    //
    eRetcode = WaveOpen((unsigned long *)pucNewSound,
                        &g_sSoundEffectHeader);

    //
    // Did we parse the sound successfully?
    //
    if(eRetcode == WAVE_OK)
    {
        //
        // Remember that we are playing a sound effect.
        //
        g_bSoundPlaying = true;

        //
        // Start playback of the wave file data.
        //
        WavePlayStart(&g_sSoundEffectHeader);
    }
}

//*****************************************************************************
//
// Main entry point for the "Blox" game.
//
//*****************************************************************************
int
main(void)
{
    unsigned long ulTimestamp, ulElapsed;
    tContext sContext;
    int iEnd;
    unsigned long ulUser0, ulUser1, ulLastIPAddr, ulIPAddr;
    unsigned char pucMACAddr[6];

    //
    // Set the system clock to run at 50MHz from the PLL
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Set the device pinout appropriately for this board.
    //
    PinoutSet();

    //
    // Enable Port F for Ethernet LEDs.
    //  LED0        Bit 3   Output
    //  LED1        Bit 2   Output
    //
    GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    //
    // Configure SysTick for a 1KHz interrupt.
    //
    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / TICKS_PER_SECOND);
    ROM_SysTickEnable();
    ROM_SysTickIntEnable();

    //
    // Enable Interrupts
    //
    ROM_IntMasterEnable();

    //
    // Initialize the SDRAM.
    //
    SDRAMInit(1, (EPI_SDRAM_CORE_FREQ_50_100 |
              EPI_SDRAM_FULL_POWER | EPI_SDRAM_SIZE_64MBIT), 1024);

    //
    // Initialize the USB keyboard functions.
    //
    USBKeyboardInit();

    //
    // Get the MAC address from the user registers in NV ram.
    //
    ROM_FlashUserGet(&ulUser0, &ulUser1);

    //
    // Convert the 24/24 split MAC address from NV ram into a MAC address
    // array.
    //
    pucMACAddr[0] = ulUser0 & 0xff;
    pucMACAddr[1] = (ulUser0 >> 8) & 0xff;
    pucMACAddr[2] = (ulUser0 >> 16) & 0xff;
    pucMACAddr[3] = ulUser1 & 0xff;
    pucMACAddr[4] = (ulUser1 >> 8) & 0xff;
    pucMACAddr[5] = (ulUser1 >> 16) & 0xff;

    //
    // Initialize the lwIP TCP/IP stack.
    //
    lwIPInit(pucMACAddr, 0, 0, 0, IPADDR_USE_DHCP);

    //
    // Setup the device locator service.
    //
    LocatorInit();
    LocatorMACAddrSet(pucMACAddr);
    LocatorAppTitleSet("RDK-IDM-SBC qs-blox");

    //
    // Start the remote software update module.
    //
    SoftwareUpdateInit(SoftwareUpdateRequestCallback);

    //
    // Initialize the web server.
    //
    BloxWebInit();

    //
    // Configure and enable uDMA
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(10);
    ROM_uDMAControlBaseSet(&sDMAControlTable[0]);
    ROM_uDMAEnable();

    //
    // Initialize the sound driver.
    //
    SoundInit();

    //
    // Set the MAC address string ready for display.
    //
    usprintf(g_pcMACAddr,
             "%02x-%02x-%02x-%02x-%02x-%02x",
             pucMACAddr[0], pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
             pucMACAddr[4], pucMACAddr[5]);

    //
    // Initialize the IP address string.
    //
    ulLastIPAddr = 0;
    usprintf(g_pcIPAddr, "");

    //
    // Initialize the display driver.
    //
    Kitronix320x240x16_SSD2119Init();

    //
    // Turn on the display backlight at full brightness.
    //
    Kitronix320x240x16_SSD2119BacklightOn(255);

    //
    // Initialize the touch screen driver.
    //
    TouchScreenInit();

    //
    // Set the touch screen event handler.
    //
    TouchScreenCallbackSet(WidgetPointerMessage);

    //
    // Add the compile-time defined widgets to the widget tree.
    //
    WidgetAdd(WIDGET_ROOT, (tWidget *)&g_sBackground);

    //
    // Decompress the JPEG image used for the application background.
    //
    JPEGWidgetImageSet((tWidget *)&g_sBackground, g_pucBackgroundJPG,
                       g_ulBackgroundJPGLen);

    //
    // Paint the widget tree to make sure they all appear on the display.
    //
    WidgetPaint(WIDGET_ROOT);

    //
    // Clear our snapshot of the system time.
    //
    ulTimestamp = 0;

    //
    // Initialize the game itself.
    //
    blox_init(0);

    //
    // Enter the main loop - we stay in the game forever from this point unless
    // someone requests a remote firmware update using the eflash or LMFlash
    // applications.
    //
    while(!g_bFirmwareUpdate)
    {
        //
        // What is our current IP address?
        //
        ulIPAddr = lwIPLocalIPAddrGet();

        //
        // If it changed, update the display.
        //
        if(ulIPAddr != ulLastIPAddr)
        {
            ulLastIPAddr = ulIPAddr;

            usprintf(g_pcIPAddr, "IP: %d.%d.%d.%d",
                     ulIPAddr & 0xff, (ulIPAddr >> 8) & 0xff,
                     (ulIPAddr >> 16) & 0xff,  ulIPAddr >> 24);

            if(g_eGameState == BLOX_WAITING)
            {
                WidgetPaint((tWidget *)&g_sIPAddr);
            }
        }

        //
        // Process any messages from or for the widgets.
        //
        WidgetMessageQueueProcess();

        //
        // Do any processing required by the USB keyboard.
        //
        USBKeyboardProcess();

        //
        // Do any audio processing that we need to do.
        //
        AudioSoundEffectProcess();

        //
        // How much time has elapsed since the last timestamp?
        //
        ulElapsed = g_ulSysTickCount - ulTimestamp;

        //
        // What state is the game currently in?
        //
        switch(g_eGameState)
        {
            case BLOX_WAITING:
                //
                // Just hang in here until someone presses the start button.
                //
                if(g_ulCommandFlags & BLOX_CMD_START)
                {
                    //
                    // Switch to starting state.
                    //
                    g_eGameState = BLOX_STARTING;

                    //
                    // Play a beep.
                    //
                    PlayNewSoundEffect(BLOX_STAT_DOWN);

                    //
                    // Clear the command flags.
                    //
                    g_ulCommandFlags = 0;

                    //
                    // Set the initial countdown.
                    //
                    g_ulCountdown = COUNTDOWN_SECONDS;

                    //
                    // Remove the Start button from the display.
                    //
                    WidgetRemove((tWidget *)&g_sStartButton);

                    //
                    // Repaint the central section of the display to show
                    // the countdown.
                    //
                    WidgetPaint((tWidget *)&g_sStoppedCanvas);

                    //
                    // Reset the timestamp.
                    //
                    ulTimestamp = g_ulSysTickCount;
                }
                break;

            case BLOX_STARTING:
                //
                // Has a second elapsed since the "Start" button was pressed?
                // If so, update the countdown.
                //
                if(ulElapsed >= TICKS_PER_SECOND)
                {
                    //
                    // Reduce the countdown.
                    //
                    g_ulCountdown--;

                    //
                    // Play a beep.
                    //
                    PlayNewSoundEffect(BLOX_STAT_DOWN);

                    //
                    // Has it reached 0?
                    //
                    if(!g_ulCountdown)
                    {
                        //
                        // Yes - set up for the new game.  First remove the
                        // widget subtree that shows when the game is stopped.
                        //
                        WidgetRemove((tWidget *)&g_sStoppedCanvas);

                        //
                        // Now add the main game display area widget.
                        //
                        WidgetAdd((tWidget *)&g_sGameBorder,
                                  (tWidget *)&g_sGameCanvas);

                        //
                        // Initialize the game itself.
                        //
                        blox_init(TimerValueGet(TIMER0_BASE, TIMER_A));

                        //
                        // Repaint the main game display area.
                        //
                        WidgetPaint((tWidget *)&g_sGameCanvas);

                        //
                        // Switch into playing mode.
                        //
                        g_eGameState = BLOX_PLAYING;

                        //
                        // Clear any control input that occurred since the last
                        // game was played.
                        //
                        g_ulCommandFlags = 0;
                    }
                    else
                    {
                        //
                        // No - update the countdown on the display and reset
                        // the timestamp to wait another second.
                        //
                        UpdateCountdown(g_ulCountdown);
                    }

                    //
                    // Reset the timestamp.
                    //
                    ulTimestamp = g_ulSysTickCount;
                }
                break;

            case BLOX_PLAYING:
                //
                // If at least 1 millisecond has gone by, process the game.
                //
                if(ulElapsed)
                {
                    //
                    // Process a step in the game.
                    //
                    iEnd = blox_timer(ulElapsed,
                                      (unsigned long *)&g_ulCommandFlags);

                    //
                    // Check to see if we need to play any new sound effect.
                    //
                    if(g_ulCommandFlags & BLOX_STAT_MASK)
                    {
                        PlayNewSoundEffect(g_ulCommandFlags);
                    }

                    //
                    // Clear the command and status flags since all of them
                    // have been handled now.
                    //
                    g_ulCommandFlags = 0;

                    //
                    // Update the timestamp to ensure that we process the game
                    // again in another millisecond.
                    //
                    ulTimestamp += ulElapsed;

                    //
                    // Did the game finish?
                    //
                    if(iEnd)
                    {
                        //
                        // Yes - switch to game over mode
                        //
                        g_eGameState = BLOX_GAME_OVER;

                        //
                        // Update the timestamp to allow us to time the "Game
                        // Over" display.
                        //
                        ulTimestamp = g_ulSysTickCount;

                        //
                        // Remove the game canvas and replace it with the
                        // stopped canvas.
                        //
                        WidgetRemove((tWidget *)&g_sGameCanvas);
                        WidgetAdd((tWidget *)&g_sGameBorder,
                                  (tWidget *)&g_sStoppedCanvas);

                        //
                        // Update the display and high score.
                        //
                        GameOver(score);
                    }
                }
                break;

            case BLOX_GAME_OVER:
                //
                // Have we been in this mode long enough?
                //
                if(ulElapsed >= GAME_OVER_DISPLAY_TIME)
                {
                    //
                    // Time to move back to waiting state.
                    //
                    g_eGameState = BLOX_WAITING;

                    //
                    // Add the "Start" button back to the tree.
                    //
                    WidgetAdd((tWidget *)&g_sStoppedCanvas,
                              (tWidget *)&g_sStartButton);
                    //
                    // Repaint the main area of the display.
                    //
                    WidgetPaint((tWidget *)&g_sStoppedCanvas);

                    //
                    // Reset the timestamp again.
                    //
                    ulTimestamp = g_ulSysTickCount;
                }
                break;
        }
    }

    //
    // If we drop out of the loop, someone requested a software update so
    // tell the user what's going on.
    //
    GrContextInit(&sContext, &g_sKitronix320x240x16_SSD2119);
    GrContextForegroundSet(&sContext, TEXT_COLOR);
    GrContextBackgroundSet(&sContext, BACKGROUND_COLOR);
    GrContextFontSet(&sContext, g_pFontCmss20);
    GrStringDrawCentered(&sContext, "  Updating Firmware...  ", -1,
                         GrContextDpyWidthGet(&sContext) / 2,
                         (GrContextDpyHeightGet(&sContext) / 2),
                         true);

    //
    // Transfer control to the bootloader.
    //
    SoftwareUpdateBegin();

    //
    // The boot loader should take control, so this should never be reached.
    // Just in case, loop forever.
    //
    while(1)
    {
    }
}
