//*****************************************************************************
//
// blox_web.c - Web server functions for the qs-blox example application.
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
// This is part of revision 9453 of the RDK-IDM-SBC Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "drivers/kitronix320x240x16_ssd2119_idm_sbc.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "third_party/blox/blox.h"
#include "blox_screen.h"
#include "string.h"
#include "cgifuncs.h"

//*****************************************************************************
//
// The handler function for the "setlevel.cgi" CGI request.
//
//*****************************************************************************
static char *SetLevelCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                                char *pcValue[]);

//*****************************************************************************
//
// CGI URI indices for each entry in the g_psConfigCGIURIs array.
//
//*****************************************************************************
#define CGI_INDEX_SETLEVEL  1

//*****************************************************************************
//
// This array is passed to the HTTPD server to inform it of special URIs
// that are treated as common gateway interface (CGI) scripts.  Each URI name
// is defined along with a pointer to the function which is to be called to
// process it.
//
//*****************************************************************************
static const tCGI g_psConfigCGIURIs[] =
{
    { "/setlevel.cgi", SetLevelCGIHandler }          // CGI_INDEX_SETLEVEL
};

//*****************************************************************************
//
//! The number of individual CGI URIs that are configured for this system.
//
//*****************************************************************************
#define NUM_CONFIG_CGI_URIS     (sizeof(g_psConfigCGIURIs) / sizeof(tCGI))

//*****************************************************************************
//
// The file sent back to the browser at the end of processing the setlevel.cgi
// CGI handler.
//
//*****************************************************************************
#define SETLEVEL_CGI_RESPONSE    "/level.ssi"

//*****************************************************************************
//
// SSI tag indices for each entry in the g_pcBloxSSITags array.
//
//*****************************************************************************
#define SSI_INDEX_SCORE       0
#define SSI_INDEX_HISCORE     1
#define SSI_INDEX_STATE       2
#define SSI_INDEX_LEVEL       3
#define SSI_INDEX_LEVELVAR    4
#define SSI_INDEX_LEVELCH     5
#define SSI_INDEX_XMLSCORE    6
#define SSI_INDEX_XMLHISCORE  7
#define SSI_INDEX_XMLSTATE    8

//*****************************************************************************
//
// This array holds all the strings that are to be recognized as SSI tag
// names by the HTTPD server.  The server will call SSIHandler to request a
// replacement string whenever the pattern <!--#tagname--> (where tagname
// appears in the following array) is found in ".ssi", ".shtml" or ".shtm"
// files that it serves.
//
//*****************************************************************************
static const char *g_pcBloxSSITags[] =
{
    "score",        // SSI_INDEX_SCORE
    "hiscore",      // SSI_INDEX_HISCORE
    "state",        // SSI_INDEX_STATE
    "level",        // SSI_INDEX_LEVEL
    "lvar",         // SSI_INDEX_LEVELVAR
    "lch",          // SSI_INDEX_LEVELCH
    "xscore",       // SSI_INDEX_XMLSCORE
    "xhiscore",     // SSI_INDEX_XMLHISCORE
    "xstate",       // SSI_INDEX_XMLSTATE
};

//*****************************************************************************
//
// The number of individual SSI tags that the HTTPD server can expect to
// find in our configuration pages.
//
//*****************************************************************************
#define NUM_BLOX_SSI_TAGS     (sizeof(g_pcBloxSSITags) / sizeof (char *))

//*****************************************************************************
//
// Definitions for the strings used to start and stop a block of JavaScript.
//
//*****************************************************************************
#define JAVASCRIPT_HEADER                                                     \
    "<script type='text/javascript' language='JavaScript'><!--\n"
#define JAVASCRIPT_FOOTER                                                     \
    "//--></script>\n"

//*****************************************************************************
//
// Difficulty levels defined as strings.
//
//*****************************************************************************
typedef struct
{
    int iLevel;
    const char *pcName;
}
tLevelNames;

const tLevelNames g_psLevelNames[] =
{
   {MINLEVEL, "Trivial"},
   {DEFAULTLEVEL, "Easy"},
   {MAXLEVEL - 4, "Tricky"},
   {MAXLEVEL, "Insane" }
};

#define NUM_LEVEL_NAMES (sizeof(g_psLevelNames) / sizeof(tLevelNames))

//*****************************************************************************
//
// This CGI handler is called whenever the web browser requests setlevel.cgi.
// It expects a single parameter, "level" with value between 0 and
// (NUM_LEVEL_NAMES - 1).
//
//*****************************************************************************
static char *
SetLevelCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    long lLevel;
    tBoolean bParamError;

    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;

    //
    // Get each of the expected parameters.
    //
    lLevel = GetCGIParam("level", pcParam, pcValue, iNumParams,
                         &bParamError);

    //
    // Was there any error reported by the parameter parser?
    //
    if(bParamError || (lLevel < 0) || (lLevel >= NUM_LEVEL_NAMES))
    {
        //
        // If so, we just exit here sending back the normal response.  This
        // will update the web page to show the current difficulty level and
        // ignore the CGI request.
        //
        return(SETLEVEL_CGI_RESPONSE);
    }

    //
    // We got the expected parameter and its value was within the valid range
    // so go ahead and change the level.
    //
    level = g_psLevelNames[lLevel].iLevel;

    //
    // Send back the response page.
    //
    return(SETLEVEL_CGI_RESPONSE);
}

//*****************************************************************************
//
// This function is called by the HTTP server whenever it encounters an SSI
// tag in a web page.  The iIndex parameter provides the index of the tag in
// the g_pcConfigSSITags array. This function writes the substitution text
// into the pcInsert array, writing no more than iInsertLen characters.
//
//*****************************************************************************
static int
BloxSSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    char *pcTemp;
    unsigned long ulLoop;
    int iUsed, iLen;

    //
    // Which SSI tag have we been passed?
    //
    switch(iIndex)
    {
        case SSI_INDEX_SCORE:
            usnprintf(pcInsert, iInsertLen, "%d", score);
            break;

        case SSI_INDEX_HISCORE:
            usnprintf(pcInsert, iInsertLen, "%d", g_iHighScore);
            break;

        case SSI_INDEX_XMLSCORE:
            usnprintf(pcInsert, iInsertLen, "\n<score>%d</score>", score);
            break;

        case SSI_INDEX_XMLHISCORE:
            usnprintf(pcInsert, iInsertLen, "\n<hiscore>%d</hiscore>",
                      g_iHighScore);
            break;

        case SSI_INDEX_XMLSTATE:
        case SSI_INDEX_STATE:
            switch(g_eGameState)
            {
                case BLOX_WAITING:
                    pcTemp = "Waiting To Start";
                    break;

                case BLOX_STARTING:
                    pcTemp = "Countdown!";
                    break;

                case BLOX_PLAYING:
                    pcTemp = "Game In Progress";
                    break;

                case BLOX_GAME_OVER:
                    pcTemp = "Game Over";
                    break;

                default:
                    pcTemp = "Unknown";
                    break;
            }
            if(iIndex == SSI_INDEX_STATE)
            {
                usnprintf(pcInsert, iInsertLen, "%s", pcTemp);
            }
            else
            {
                usnprintf(pcInsert, iInsertLen, "\n<bloxstate>%s</bloxstate>",
                          pcTemp);
            }
            break;

        case SSI_INDEX_LEVEL:
            //
            // Loop through the levels defined.
            //
            for(ulLoop = 0; ulLoop < NUM_LEVEL_NAMES; ulLoop++)
            {
                //
                // Have we found the name for the current level?
                //
                if(g_psLevelNames[ulLoop].iLevel == level)
                {
                    //
                    // Yes - copy it into the output buffer and drop out of the
                    // loop.
                    //
                    usnprintf(pcInsert, iInsertLen, "%s",
                              g_psLevelNames[ulLoop].pcName);
                    break;
                }
            }

            //
            // If we drop out, someone selected a level that we don't have
            // a name for so just show the level number.
            //
            if(ulLoop == NUM_LEVEL_NAMES)
            {
                usnprintf(pcInsert, iInsertLen, "Level %d", level);
            }
            break;

        //
        // The current game level defined as a JavaScript variable.
        //
        case SSI_INDEX_LEVELVAR:
            //
            // Loop through the levels defined.
            //
            for(ulLoop = 0; ulLoop < NUM_LEVEL_NAMES; ulLoop++)
            {
                //
                // Have we found the entry for the current level?
                //
                if(g_psLevelNames[ulLoop].iLevel >= level)
                {
                    break;
                }
            }

            usnprintf(pcInsert, iInsertLen,
                      "%slevel=%d;\n%s",
                      JAVASCRIPT_HEADER,
                      ulLoop,
                      JAVASCRIPT_FOOTER);
            break;

        //
        // The possible game levels defined as a list of choices for an HTML
        // form list box.
        //
        case SSI_INDEX_LEVELCH:

            //
            // Set up for the loop.
            //
            pcTemp = pcInsert;
            iUsed = 0;

            //
            // Loop through the various choices.
            //
            for(ulLoop = 0; ulLoop < NUM_LEVEL_NAMES; ulLoop++)
            {
                //
                // Write one option string to the output.
                //
                iLen = usnprintf(pcTemp, iInsertLen - iUsed,
                                 "<option value=""%d"">%s</option>\n",
                                 ulLoop,
                                 g_psLevelNames[ulLoop].pcName);

                //
                // Did we run out of buffer space?
                //
                if((iLen < 0) || (iLen >= (iInsertLen - iUsed)))
                {
                    //
                    // Yes - terminate the string before the option whose
                    // formatting failed then exit the loop.
                    //
                    *pcTemp = (char)0;
                    break;
                }

                //
                // Skip past the string we just formatted.
                //
                iUsed += iLen;
                pcTemp += iLen;
            }
            break;

        default:
            usnprintf(pcInsert, iInsertLen, "??");
            break;
    }

    //
    // Tell the server how many characters our insert string contains.
    //
    return(strlen(pcInsert));
}

//*****************************************************************************
//
// Initialize the web server, CGI and SSI handlers.
//
//*****************************************************************************
void
BloxWebInit(void)
{
    //
    // Initialize the httpd server.
    //
    httpd_init();

    //
    // Pass our tag information to the HTTP server.
    //
    http_set_ssi_handler(BloxSSIHandler, g_pcBloxSSITags, NUM_BLOX_SSI_TAGS);

    //
    // Hook our CGI handler to the HTTP server.
    //
    http_set_cgi_handlers(g_psConfigCGIURIs, NUM_CONFIG_CGI_URIS);
}
