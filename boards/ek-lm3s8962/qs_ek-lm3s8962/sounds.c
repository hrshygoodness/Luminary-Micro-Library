//*****************************************************************************
//
// sounds.c - Arrays containing the music and sound effects used by the
//            application.
//
// Copyright (c) 2007-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S8962 Firmware Package.
//
//*****************************************************************************

#include "audio.h"
#include "sounds.h"

//*****************************************************************************
//
// The music played during the splash screens.
//
//*****************************************************************************
const unsigned short g_pusIntro[80] =
{
    0, B4,
    253, SILENCE,
    263, G5,
    515, SILENCE,
    525, FS5,
    712, SILENCE,
    722, E5,
    778, SILENCE,
    788, D5,
    974, SILENCE,
    984, A4,
    1040, SILENCE,
    1050, B4,
    1303, SILENCE,
    1313, G5,
    1565, SILENCE,
    1575, FS5,
    1762, SILENCE,
    1772, E5,
    1828, SILENCE,
    1838, D5,
    2024, SILENCE,
    2034, A4,
    2090, SILENCE,
    2100, B4,
    2353, SILENCE,
    2363, G5,
    2615, SILENCE,
    2625, E5,
    2812, SILENCE,
    2822, D5,
    2878, SILENCE,
    2888, C5,
    3140, SILENCE,
    3150, A4,
    3403, SILENCE,
    3413, D5,
    3665, SILENCE,
    3675, B4,
    4200, 0
};

//*****************************************************************************
//
// The music played at the start of the game.
//
//*****************************************************************************
const unsigned short g_pusStartOfGame[24] =
{
    0, C4,
    40, SILENCE,
    50, E4,
    90, SILENCE,
    100, G4,
    140, SILENCE,
    150, C5,
    240, SILENCE,
    250, G4,
    290, SILENCE,
    300, C5,
    550, 0
};

//*****************************************************************************
//
// The music played when a maze has been completed.
//
//*****************************************************************************
const unsigned short g_pusEndOfMaze[28] =
{
    0, E4,
    40, SILENCE,
    50, D4,
    90, SILENCE,
    100, C4,
    140, SILENCE,
    150, F4,
    190, SILENCE,
    200, E4,
    240, SILENCE,
    250, D4,
    290, SILENCE,
    300, G4,
    550, 0
};

//*****************************************************************************
//
// The music played at the end of the game, after the player dies.
//
//*****************************************************************************
const unsigned short g_pusEndOfGame[36] =
{
    0, A3,
    152, SILENCE,
    162, A3,
    315, SILENCE,
    325, A3,
    477, SILENCE,
    487, E3,
    559, SILENCE,
    569, B3,
    640, SILENCE,
    650, A3,
    802, SILENCE,
    812, E3,
    884, SILENCE,
    894, B3,
    965, SILENCE,
    975, A3,
    1300, 0
};

//*****************************************************************************
//
// The sound effect played when a bullet is fired by the player.
//
//*****************************************************************************
const unsigned short g_pusFireEffect[30] =
{
    500, 510, 520, 530, 540, 550, 560, 570, 580, 590, 600, 610, 620, 630, 640,
    650, 660, 670, 680, 690, 700, 710, 720, 730, 740, 750, 760, 770, 780, 790
};

//*****************************************************************************
//
// The sound effect played when a bullet collides with a wall.
//
//*****************************************************************************
const unsigned short g_pusWallEffect[100] =
{
    100, 100, 100, 100, 100, 100, 100, 100, 110, 110, 110, 110, 110, 110, 110,
    110, 100, 100, 100, 100, 100, 100, 100, 100,  90,  90,  90,  90,  90,  90,
     90,  90, 100, 100, 100, 100, 100, 100, 100, 100, 110, 110, 110, 110, 110,
    110, 110, 110, 100, 100, 100, 100, 100, 100, 100, 100,  90,  90,  90,  90,
     90,  90,  90,  90, 100, 100, 100, 100, 100, 100, 100, 100, 110, 110, 110,
    110, 110, 110, 110, 110, 100, 100, 100, 100, 100, 100, 100, 100,  90,  90,
     90,  90,  90,  90,  90,  90, 100, 100, 100, 100
};

//*****************************************************************************
//
// The sound effect played when a monster dies.
//
//*****************************************************************************
const unsigned short g_pusMonsterEffect[100] =
{
    1500, 1490, 1480, 1470, 1460, 1450, 1440, 1430, 1420, 1410,
    1400, 1390, 1380, 1370, 1360, 1350, 1340, 1330, 1320, 1310,
    1300, 1290, 1280, 1270, 1260, 1250, 1240, 1230, 1220, 1210,
    1200, 1190, 1180, 1170, 1160, 1150, 1140, 1130, 1120, 1110,
    1100, 1090, 1080, 1070, 1060, 1050, 1040, 1030, 1020, 1010,
    1000,  990,  980,  970,  960,  950,  940,  930,  920,  910,
     900,  890,  880,  870,  860,  850,  840,  830,  820,  810,
     800,  790,  780,  770,  760,  750,  740,  730,  720,  710,
     700,  690,  680,  670,  660,  650,  640,  630,  620,  610,
     600,  590,  580,  570,  560,  550,  540,  530,  520,  510
};

//*****************************************************************************
//
// The sound effect played when the player dies.
//
//*****************************************************************************
const unsigned short g_pusPlayerEffect[150] =
{
     100,  110,  120,  130,  140,  150,  160,  170,  180,  190,
     200,  210,  220,  230,  240,  250,  260,  270,  280,  290,
     300,  310,  320,  330,  340,  350,  360,  370,  380,  390,
     400,  410,  420,  430,  440,  450,  460,  470,  480,  490,
     500,  510,  520,  530,  540,  550,  560,  570,  580,  590,
     600,  610,  620,  630,  640,  650,  660,  670,  680,  690,
     700,  710,  720,  730,  740,  750,  760,  770,  780,  790,
     800,  810,  820,  830,  840,  850,  860,  870,  880,  890,
     900,  910,  920,  930,  940,  950,  960,  970,  980,  990,
    1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070, 1080, 1090,
    1100, 1110, 1120, 1130, 1140, 1150, 1160, 1170, 1180, 1190,
    1200, 1210, 1220, 1230, 1240, 1250, 1260, 1270, 1280, 1290,
    1300, 1310, 1320, 1330, 1340, 1350, 1360, 1370, 1380, 1390,
    1400, 1410, 1420, 1430, 1440, 1450, 1460, 1470, 1480, 1490,
    1500, 1510, 1520, 1530, 1540, 1550, 1560, 1570, 1580, 1590
};
