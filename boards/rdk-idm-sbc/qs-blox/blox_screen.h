//*****************************************************************************
//
// blox_screen.h - Screen handling functions used by the Blox game.
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

#ifndef __BLOX_SCREEN_H__
#define __BLOX_SCREEN_H__

//*****************************************************************************
//
// The four states that the game can be in.
//
//*****************************************************************************
typedef enum
{
    BLOX_WAITING,
    BLOX_STARTING,
    BLOX_PLAYING,
    BLOX_GAME_OVER
}
tGameState;

extern tGameState g_eGameState;

//*****************************************************************************
//
// An additional, application-specific command flag used to indicate that the
// user wishes to start a new game.  Note that other command flags are defined
// in common header blox.h.
//
//*****************************************************************************
#define BLOX_CMD_START 0x80000000

//*****************************************************************************
//
// The number of seconds in the countdown after pressing "Start" and before
// the game commences.
//
//*****************************************************************************
#define COUNTDOWN_SECONDS 3

//*****************************************************************************
//
// The number of SysTick interrupts we want per second.
//
//*****************************************************************************
#define TICKS_PER_SECOND 1000

//*****************************************************************************
//
// The number of milliseconds to show the "Game Over" screen before moving
// back to the start screen.
//
//*****************************************************************************
#define GAME_OVER_DISPLAY_TIME (5 * TICKS_PER_SECOND)

//*****************************************************************************
//
// The number of characters in the game score buffer.
//
//*****************************************************************************
#define MAX_SCORE_LEN 12

//*****************************************************************************
//
// Colors for various elements.
//
//*****************************************************************************
#define BACKGROUND_COLOR ClrBlack
#define TEXT_COLOR       ClrWhite
#define HIGHLIGHT_COLOR  ClrRed
#define SHADOW_COLOR     ClrWhite
#define BORDER_COLOR     ClrWhite
#define SCORE_COLOR      ClrRed
#define MESSAGE_COLOR    ClrYellow

//*****************************************************************************
//
// Coordinates and size of the game area and the size of each of the blocks.
//
//*****************************************************************************
#define GAME_AREA_LEFT    20
#define GAME_AREA_TOP    100
#define GAME_AREA_WIDTH  240
#define GAME_AREA_HEIGHT 120
#define GAME_BLOCK_SIZE   12

//*****************************************************************************
//
// Various widgets that the blox_screen module needs to know about.  These are
// defined in qs_blox.c.
//
//*****************************************************************************
extern tCanvasWidget g_sGameCanvas;
extern tCanvasWidget g_sScore;
extern tCanvasWidget g_sNextPiece;
extern tCanvasWidget g_sStoppedCanvas;

//*****************************************************************************
//
// The string buffer for the current score.
//
//*****************************************************************************
extern char g_pcScore[];

//*****************************************************************************
//
// The highest score obtained since the application started.
//
//*****************************************************************************
extern int g_iHighScore;

//*****************************************************************************
//
// The countdown value displayed prior to starting a new game.
//
//*****************************************************************************
extern unsigned long g_ulCountdown;

//*****************************************************************************
//
// Functions exported from the blox_screen file.
//
//*****************************************************************************
extern void OnGameAreaPaint(tWidget *pWidget, tContext *pContext);
extern void OnNextPiecePaint(tWidget *pWidget, tContext *pContext);
extern void OnStopAreaPaint(tWidget *pWidget, tContext *pContext);
extern void GameOver(int iLastScore);
extern void UpdateCountdown(unsigned long ulCountdown);

#endif // __BLOX_SCREEN_H__
