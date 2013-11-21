//*****************************************************************************
//
// config.c - Configuration of the serial to Ethernet converter.
//
// Copyright (c) 2008-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the RDK-S2E Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/uart.h"
#include "utils/flash_pb.h"
#include "utils/locator.h"
#include "utils/lwiplib.h"
#include "utils/ustdlib.h"
#include "httpserver_raw/httpd.h"
#include "config.h"
#include "serial.h"
#include "telnet.h"
#include "upnp.h"

//*****************************************************************************
//
//! \addtogroup config_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
//! A local flag indicating that a firmware update has been requested via the
//! web-based configuration pages.
//
//*****************************************************************************
static tBoolean g_bUpdateRequested = false;

//*****************************************************************************
//
//! A flag to the main loop indicating that it should enter the bootloader and
//! perform a firmware update.
//
//*****************************************************************************
tBoolean g_bStartBootloader = false;

//*****************************************************************************
//
//! Flags sent to the main loop indicating that it should update the IP address
//! after a short delay (to allow us to send a suitable page back to the web
//! browser telling it the address has changed).
//
//*****************************************************************************
unsigned char g_cUpdateRequired;

//*****************************************************************************
//
//! The maximum length of any HTML form variable name used in this application.
//
//*****************************************************************************
#define MAX_VARIABLE_NAME_LEN   16

//*****************************************************************************
//
// SSI tag indices for each entry in the g_pcSSITags array.
//
//*****************************************************************************
#define SSI_INDEX_IPADDR        0
#define SSI_INDEX_MACADDR       1
#define SSI_INDEX_DOUPDATE      2
#define SSI_INDEX_P0BR          3
#define SSI_INDEX_P0SB          4
#define SSI_INDEX_P0P           5
#define SSI_INDEX_P0BC          6
#define SSI_INDEX_P0FC          7
#define SSI_INDEX_P0TT          8
#define SSI_INDEX_P0TLP         9
#define SSI_INDEX_P0TRP         10
#define SSI_INDEX_P0TIP         11
#define SSI_INDEX_P0TIP1        12
#define SSI_INDEX_P0TIP2        13
#define SSI_INDEX_P0TIP3        14
#define SSI_INDEX_P0TIP4        15
#define SSI_INDEX_P0TNM         16
#define SSI_INDEX_P1BR          17
#define SSI_INDEX_P1SB          18
#define SSI_INDEX_P1P           19
#define SSI_INDEX_P1BC          20
#define SSI_INDEX_P1FC          21
#define SSI_INDEX_P1TT          22
#define SSI_INDEX_P1TLP         23
#define SSI_INDEX_P1TRP         24
#define SSI_INDEX_P1TIP         25
#define SSI_INDEX_P1TIP1        26
#define SSI_INDEX_P1TIP2        27
#define SSI_INDEX_P1TIP3        28
#define SSI_INDEX_P1TIP4        29
#define SSI_INDEX_P1TNM         30
#define SSI_INDEX_MODNAME       31
#define SSI_INDEX_PNPPORT       32
#define SSI_INDEX_DISABLE       33
#define SSI_INDEX_DVARS         34
#define SSI_INDEX_P0VARS        35
#define SSI_INDEX_P1VARS        36
#define SSI_INDEX_MODNINP       37
#define SSI_INDEX_PNPINP        38
#define SSI_INDEX_P0TVARS       39
#define SSI_INDEX_P1TVARS       40
#define SSI_INDEX_P0IPVAR       41
#define SSI_INDEX_P1IPVAR       42
#define SSI_INDEX_IPVARS        43
#define SSI_INDEX_SNVARS        44
#define SSI_INDEX_GWVARS        45
#define SSI_INDEX_REVISION      46
#define SSI_INDEX_P0PROT        47
#define SSI_INDEX_P1PROT        48

//*****************************************************************************
//
//! This array holds all the strings that are to be recognized as SSI tag
//! names by the HTTPD server.  The server will call ConfigSSIHandler to
//! request a replacement string whenever the pattern <!--#tagname--> (where
//! tagname appears in the following array) is found in ``.ssi'' or ``.shtm''
//! files that it serves.
//
//*****************************************************************************
static const char *g_pcConfigSSITags[] =
{
    "ipaddr",     // SSI_INDEX_IPADDR
    "macaddr",    // SSI_INDEX_MACADDR
    "doupdate",   // SSI_INDEX_DOUPDATE
    "p0br",       // SSI_INDEX_P0BR
    "p0sb",       // SSI_INDEX_P0SB
    "p0p",        // SSI_INDEX_P0P
    "p0bc",       // SSI_INDEX_P0BC
    "p0fc",       // SSI_INDEX_P0FC
    "p0tt",       // SSI_INDEX_P0TT
    "p0tlp",      // SSI_INDEX_P0TLP
    "p0trp",      // SSI_INDEX_P0TRP
    "p0tip",      // SSI_INDEX_P0TIP
    "p0tip1",     // SSI_INDEX_P0TIP1
    "p0tip2",     // SSI_INDEX_P0TIP2
    "p0tip3",     // SSI_INDEX_P0TIP3
    "p0tip4",     // SSI_INDEX_P0TIP4
    "p0tnm",      // SSI_INDEX_P0TNM
    "p1br",       // SSI_INDEX_P1BR
    "p1sb",       // SSI_INDEX_P1SB
    "p1p",        // SSI_INDEX_P1P
    "p1bc",       // SSI_INDEX_P1BC
    "p1fc",       // SSI_INDEX_P1FC
    "p1tt",       // SSI_INDEX_P1TT
    "p1tlp",      // SSI_INDEX_P1TLP
    "p1trp",      // SSI_INDEX_P1TRP
    "p1tip",      // SSI_INDEX_P1TIP
    "p1tip1",     // SSI_INDEX_P1TIP1
    "p1tip2",     // SSI_INDEX_P1TIP2
    "p1tip3",     // SSI_INDEX_P1TIP3
    "p1tip4",     // SSI_INDEX_P1TIP4
    "p1tnm",      // SSI_INDEX_P1TNM
    "modname",    // SSI_INDEX_MODNAME
    "pnpport",    // SSI_INDEX_PNPPORT
    "disable",    // SSI_INDEX_DISABLE
    "dvars",      // SSI_INDEX_DVARS
    "p0vars",     // SSI_INDEX_P0VARS
    "p1vars",     // SSI_INDEX_P1VARS
    "modninp",    // SSI_INDEX_MODNINP
    "pnpinp",     // SSI_INDEX_PNPINP
    "p0tvars",    // SSI_INDEX_P0TVARS
    "p1tvars",    // SSI_INDEX_P1TVARS
    "p0ipvar",    // SSI_INDEX_P0IPVAR
    "p1ipvar",    // SSI_INDEX_P1IPVAR
    "ipvars",     // SSI_INDEX_IPVARS
    "snvars",     // SSI_INDEX_SNVARS
    "gwvars",     // SSI_INDEX_GWVARS
    "revision",   // SSI_INDEX_REVISION
    "p0prot",     // SSI_INDEX_P0PROT
    "p1prot"      // SSI_INDEX_P1PROT
};

//*****************************************************************************
//
//! The number of individual SSI tags that the HTTPD server can expect to
//! find in our configuration pages.
//
//*****************************************************************************
#define NUM_CONFIG_SSI_TAGS     (sizeof(g_pcConfigSSITags) / sizeof (char *))

//*****************************************************************************
//
//! Prototype for the function which handles requests for config.cgi.
//
//*****************************************************************************
static char *ConfigCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                              char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for misc.cgi.
//
//*****************************************************************************
static char *ConfigMiscCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                                  char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for ip.cgi.
//
//*****************************************************************************
static char *ConfigIPCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                                char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for update.cgi.
//
//*****************************************************************************
static char *ConfigUpdateCGIHandler(int iIndex, int iNumParams,
                                    char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the function which handles requests for defaults.cgi.
//
//*****************************************************************************
static char *ConfigDefaultsCGIHandler(int iIndex, int iNumParams,
                                      char *pcParam[], char *pcValue[]);

//*****************************************************************************
//
//! Prototype for the main handler used to process server-side-includes for the
//! application's web-based configuration screens.
//
//*****************************************************************************
static int ConfigSSIHandler(int iIndex, char *pcInsert, int iInsertLen);

//*****************************************************************************
//
// CGI URI indices for each entry in the g_psConfigCGIURIs array.
//
//*****************************************************************************
#define CGI_INDEX_CONFIG        0
#define CGI_INDEX_MISC          1
#define CGI_INDEX_UPDATE        2
#define CGI_INDEX_DEFAULTS      3
#define CGI_INDEX_IP            4

//*****************************************************************************
//
//! This array is passed to the HTTPD server to inform it of special URIs
//! that are treated as common gateway interface (CGI) scripts.  Each URI name
//! is defined along with a pointer to the function which is to be called to
//! process it.
//
//*****************************************************************************
static const tCGI g_psConfigCGIURIs[] =
{
    { "/config.cgi", ConfigCGIHandler },            // CGI_INDEX_CONFIG
    { "/misc.cgi", ConfigMiscCGIHandler },          // CGI_INDEX_MISC
    { "/update.cgi", ConfigUpdateCGIHandler },      // CGI_INDEX_UPDATE
    { "/defaults.cgi", ConfigDefaultsCGIHandler },  // CGI_INDEX_DEFAULTS
    { "/ip.cgi", ConfigIPCGIHandler },              // CGI_INDEX_IP
};

//*****************************************************************************
//
//! The number of individual CGI URIs that are used by our configuration
//! web pages.
//
//*****************************************************************************
#define NUM_CONFIG_CGI_URIS     (sizeof(g_psConfigCGIURIs) / sizeof(tCGI))

//*****************************************************************************
//
//! The file sent back to the browser by default following completion of any
//! of our CGI handlers.
//
//*****************************************************************************
#define DEFAULT_CGI_RESPONSE    "/s2e.shtml"

//*****************************************************************************
//
//! The file sent back to the browser in cases where a parameter error is
//! detected by one of the CGI handlers.  This should only happen if someone
//! tries to access the CGI directly via the browser command line and doesn't
//! enter all the required parameters alongside the URI.
//
//*****************************************************************************
#define PARAM_ERROR_RESPONSE    "/perror.shtml"

//*****************************************************************************
//
//! The file sent back to the browser to signal that the bootloader is being
//! entered to perform a software update.
//
//*****************************************************************************
#define FIRMWARE_UPDATE_RESPONSE "/blstart.shtml"

//*****************************************************************************
//
//! The file sent back to the browser to signal that the IP address of the
//! device is about to change and that the web server is no longer operating.
//
//*****************************************************************************
#define IP_UPDATE_RESPONSE  "/ipchg.shtml"

//*****************************************************************************
//
//! The URI of the ``Miscellaneous Settings'' page offered by the web server.
//
//*****************************************************************************
#define MISC_PAGE_URI           "/misc.shtml"

//*****************************************************************************
//
// Strings used for format JavaScript parameters for use by the configuration
// web pages.
//
//*****************************************************************************
#define JAVASCRIPT_HEADER                                                     \
    "<script type='text/javascript' language='JavaScript'><!--\n"
#define SER_JAVASCRIPT_VARS                                                   \
    "var br = %d;\n"                                                          \
    "var sb = %d;\n"                                                          \
    "var bc = %d;\n"                                                          \
    "var fc = %d;\n"                                                          \
    "var par = %d;\n"
#define TN_JAVASCRIPT_VARS                                                    \
    "var tt = %d;\n"                                                          \
    "var tlp = %d;\n"                                                         \
    "var trp = %d;\n"                                                         \
    "var tnm = %d;\n"                                                         \
    "var tnp = %d;\n"
#define TIP_JAVASCRIPT_VARS                                                   \
    "var tip1 = %d;\n"                                                        \
    "var tip2 = %d;\n"                                                        \
    "var tip3 = %d;\n"                                                        \
    "var tip4 = %d;\n"
#define IP_JAVASCRIPT_VARS                                                    \
    "var staticip = %d;\n"                                                    \
    "var sip1 = %d;\n"                                                        \
    "var sip2 = %d;\n"                                                        \
    "var sip3 = %d;\n"                                                        \
    "var sip4 = %d;\n"
#define SUBNET_JAVASCRIPT_VARS                                                \
    "var mip1 = %d;\n"                                                        \
    "var mip2 = %d;\n"                                                        \
    "var mip3 = %d;\n"                                                        \
    "var mip4 = %d;\n"
#define GW_JAVASCRIPT_VARS                                                    \
    "var gip1 = %d;\n"                                                        \
    "var gip2 = %d;\n"                                                        \
    "var gip3 = %d;\n"                                                        \
    "var gip4 = %d;\n"
#define JAVASCRIPT_FOOTER                                                     \
    "//--></script>\n"

//*****************************************************************************
//
//! Structure used in mapping numeric IDs to human-readable strings.
//
//*****************************************************************************
typedef struct
{
    //
    //! A human readable string related to the identifier found in the ucId
    //! field.
    //
    const char *pcString;

    //
    //! An identifier value associated with the string held in the pcString
    //! field.
    //
    unsigned char ucId;
}
tStringMap;

//*****************************************************************************
//
//! The array used to map between parity identifiers and their human-readable
//! equivalents.
//
//*****************************************************************************
static const tStringMap g_psParityMap[] =
{
   { "None", SERIAL_PARITY_NONE },
   { "Odd", SERIAL_PARITY_ODD },
   { "Even", SERIAL_PARITY_EVEN },
   { "Mark", SERIAL_PARITY_MARK },
   { "Space", SERIAL_PARITY_SPACE }
};

#define NUM_PARITY_MAPS         (sizeof(g_psParityMap) / sizeof(tStringMap))

//*****************************************************************************
//
//! The array used to map between flow control identifiers and their
//! human-readable equivalents.
//
//*****************************************************************************
static const tStringMap g_psFlowControlMap[] =
{
   { "None", SERIAL_FLOW_CONTROL_NONE },
   { "Hardware", SERIAL_FLOW_CONTROL_HW }
};

#define NUM_FLOW_CONTROL_MAPS   (sizeof(g_psFlowControlMap) / \
                                 sizeof(tStringMap))

//*****************************************************************************
//
//! This structure instance contains the factory-default set of configuration
//! parameters for S2E module.
//
//*****************************************************************************
static const tConfigParameters g_sParametersFactory =
{
    //
    // The sequence number (ucSequenceNum); this value is not important for
    // the copy in SRAM.
    //
    0,

    //
    // The CRC (ucCRC); this value is not important for the copy in SRAM.
    //
    0,

    //
    // The parameter block version number (ucVersion).
    //
    0,

    //
    // Flags (ucFlags)
    //
    0,

    //
    // The TCP port number for UPnP discovery/response (usLocationURLPort).
    //
    6432,

    //
    // Reserved (ucReserved1).
    //
    {
        0, 0
    },

    //
    // Port Parameter Array.
    //
    {
        //
        // Parameters for Port 0 (sPort[0]).
        //
        {
            //
            // The baud rate (ulBaudRate).
            //
            115200,

            //
            // The number of data bits.
            //
            8,

            //
            // The parity (NONE).
            //
            SERIAL_PARITY_NONE,

            //
            // The number of stop bits.
            //
            1,

            //
            // The flow control (NONE).
            //
            SERIAL_FLOW_CONTROL_NONE,

            //
            // The telnet session timeout (ulTelnetTimeout).
            //
            0,

            //
            // The telnet session listen or local port number
            // (usTelnetLocalPort).
            //
            23,

            //
            // The telnet session remote port number (when in client mode).
            //
            23,

            //
            // The IP address to which a connection will be made when in telnet
            // client mode.  This defaults to an invalid address since it's not
            // sensible to have a default value here.
            //
            0x00000000,

            //
            // Flags indicating the operating mode for this port.
            //
            PORT_TELNET_SERVER,

            //
            // Reserved (ucReserved0).
            //
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
        },

        //
        // Parameters for Port 1 (sPort[1]).
        //
        {
            //
            // The baud rate (ulBaudRate).
            //
            115200,

            //
            // The number of data bits.
            //
            8,

            //
            // The parity (NONE).
            //
            SERIAL_PARITY_NONE,

            //
            // The number of stop bits.
            //
            1,

            //
            // The flow control (NONE).
            //
            SERIAL_FLOW_CONTROL_NONE,

            //
            // The telnet session timeout (ulTelnetTimeout).
            //
            0,

            //
            // The telnet session listen or local port number
            // (usTelnetLocalPort).
            //
            26,

            //
            // The telnet session remote port number (when in client mode).
            //
            23,

            //
            // The IP address to which a connection will be made when in telnet
            // client mode.  This defaults to an invalid address since it's not
            // sensible to have a default value here.
            //
            0x00000000,

            //
            // Flags indicating the operating mode for this port.
            //
            PORT_TELNET_SERVER,

            //
            // Reserved (ucReserved0).
            //
            {
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            },
        },
    },

    //
    // Module Name (ucModName).
    //
    {
        'T','I',' ','S','t','e','l','l','a','r','i','s',' ','S','e','r',
        'i','a','l','2','E','t','h','e','r','n','e','t',' ','M','o','d',
        'u','l','e', 0,  0,  0,  0,  0,
    },

    //
    // Static IP address (used only if indicated in ucFlags).
    //
    0x00000000,

    //
    // Default gateway IP address (used only if static IP is in use).
    //
    0x00000000,

    //
    // Subnet mask (used only if static IP is in use).
    //
    0xFFFFFF00,

    //
    // ucReserved2 (compiler will pad to the full length)
    //
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
};

//*****************************************************************************
//
//! This structure instance contains the run-time set of configuration
//! parameters for S2E module.  This is the active parameter set and may
//! contain changes that are not to be committed to flash.
//
//*****************************************************************************
tConfigParameters g_sParameters;

//*****************************************************************************
//
//! This structure instance points to the most recently saved parameter block
//! in flash.  It can be considered the default set of parameters.
//
//*****************************************************************************
const tConfigParameters *g_psDefaultParameters;

//*****************************************************************************
//
//! This structure contains the latest set of parameter committed to flash
//! and is used by the configuration pages to store changes that are to be
//! written back to flash.  Note that g_sParameters may contain other changes
//! which are not to be written so we can't merely save the contents of the
//! active parameter block if the user requests some change to the defaults.
//
//*****************************************************************************
static tConfigParameters g_sWorkingDefaultParameters;

//*****************************************************************************
//
//! This structure instance points to the factory default set of parameters in
//! flash memory.
//
//*****************************************************************************
const tConfigParameters *const g_psFactoryParameters = &g_sParametersFactory;

//*****************************************************************************
//
//! The version of the firmware.  Changing this value will make it much more
//! difficult for Texas Instruments support personnel to determine the firmware
//! in use when trying to provide assistance; it should only be changed after
//! careful consideration.
//
//*****************************************************************************
const unsigned short g_usFirmwareVersion = 10636;

//*****************************************************************************
//
//! Loads the S2E parameter block from factory-default table.
//!
//! This function is called to load the factory default parameter block.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoadFactory(void)
{
    //
    // Copy the factory default parameter set to the active and working
    // parameter blocks.
    //
    g_sParameters = g_sParametersFactory;
    g_sWorkingDefaultParameters = g_sParametersFactory;
}

//*****************************************************************************
//
//! Loads the S2E parameter block from flash.
//!
//! This function is called to load the most recently saved parameter block
//! from flash.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigLoad(void)
{
    unsigned char *pucBuffer;

    //
    // Get a pointer to the latest parameter block in flash.
    //
    pucBuffer = FlashPBGet();

    //
    // See if a parameter block was found in flash.
    //
    if(pucBuffer)
    {
        //
        // A parameter block was found so copy the contents to both our
        // active parameter set and the working default set.
        //
        g_sParameters = *(tConfigParameters *)pucBuffer;
        g_sWorkingDefaultParameters = g_sParameters;
    }
}

//*****************************************************************************
//
//! Saves the S2E parameter block to flash.
//!
//! This function is called to save the current S2E configuration parameter
//! block to flash memory.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigSave(void)
{
    unsigned char *pucBuffer;

    //
    // Save the working defaults parameter block to flash.
    //
    FlashPBSave((unsigned char *)&g_sWorkingDefaultParameters);

    //
    // Get the pointer to the most recenly saved buffer.
    // (should be the one we just saved).
    //
    pucBuffer = FlashPBGet();

    //
    // Update the default parameter pointer.
    //
    if(pucBuffer)
    {
        g_psDefaultParameters = (tConfigParameters *)pucBuffer;
    }
    else
    {
        g_psDefaultParameters = (tConfigParameters *)g_psFactoryParameters;
    }
}

//*****************************************************************************
//
//! Initializes the configuration parameter block.
//!
//! This function initializes the configuration parameter block.  If the
//! version number of the parameter block stored in flash is older than
//! the current revision, new parameters will be set to default values as
//! needed.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigInit(void)
{
    unsigned char *pucBuffer;

    //
    // Verify that the parameter block structure matches the FLASH parameter
    // block size.
    //
    ASSERT(sizeof(tConfigParameters) == FLASH_PB_SIZE)

    //
    // Initialize the flash parameter block driver.
    //
    FlashPBInit(FLASH_PB_START, FLASH_PB_END, FLASH_PB_SIZE);

    //
    // First, load the factory default values.
    //
    ConfigLoadFactory();

    //
    // Then, if available, load the latest non-volatile set of values.
    //
    ConfigLoad();

    //
    // Get the pointer to the most recently saved buffer.
    //
    pucBuffer = FlashPBGet();

    //
    // Update the default parameter pointer.
    //
    if(pucBuffer)
    {
        g_psDefaultParameters = (tConfigParameters *)pucBuffer;
    }
    else
    {
        g_psDefaultParameters = (tConfigParameters *)g_psFactoryParameters;
    }
}

//*****************************************************************************
//
//! Configures HTTPD server SSI and CGI capabilities for our configuration
//! forms.
//!
//! This function informs the HTTPD server of the server-side-include tags
//! that we will be processing and the special URLs that are used for
//! CGI processing for the web-based configuration forms.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigWebInit(void)
{
    //
    // Pass our tag information to the HTTP server.
    //
    http_set_ssi_handler(ConfigSSIHandler, g_pcConfigSSITags,
                         NUM_CONFIG_SSI_TAGS);

    //
    // Pass our CGI handlers to the HTTP server.
    //
    http_set_cgi_handlers(g_psConfigCGIURIs, NUM_CONFIG_CGI_URIS);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches a mapping array to find a human-readable description for a
//! given identifier.
//!
//! \param psMap points to an array of \e tStringMap structures which contain
//! the mappings to be searched for the provided identifier.
//! \param ulEntries contains the number of map entries in the \e psMap array.
//! \param ucId is the identifier whose description is to be returned.
//!
//! This function scans the given array of ID/string maps and returns a pointer
//! to the string associated with the /e ucId parameter passed.  If the
//! identifier is not found in the map array, a pointer to ``**UNKNOWN**'' is
//! returned.
//!
//! \return Returns a pointer to an ASCII string describing the identifier
//! passed, if found, or ``**UNKNOWN**'' if not found.
//
//*****************************************************************************
static const char *
ConfigMapIdToString(const tStringMap *psMap, unsigned long ulEntries,
                    unsigned char ucId)
{
    unsigned long ulLoop;

    //
    // Check each entry in the map array looking for the ID number we were
    // passed.
    //
    for(ulLoop = 0; ulLoop < ulEntries; ulLoop++)
    {
        //
        // Does this map entry match?
        //
        if(psMap[ulLoop].ucId == ucId)
        {
            //
            // Yes - return the IDs description string.
            //
            return(psMap[ulLoop].pcString);
        }
    }

    //
    // If we drop out, the ID passed was not found in the map array so return
    // a default "**UNKNOWN**" string.
    //
    return("**UNKNOWN**");
}

//*****************************************************************************
//
//! \internal
//!
//! Updates all parameters associated with a single port.
//!
//! \param ulPort specifies which of the two supported ports to update.  Valid
//! values are 0 and 1.
//!
//! This function changes the serial and telnet configuration to match the
//! values stored in g_sParameters.sPort for the supplied port.  On exit, the
//! new parameters will be in effect and g_sParameters.sPort will have been
//! updated to show the actual parameters in effect (in case any of the
//! supplied parameters are not valid or the actual hardware values differ
//! slightly from the requested value).
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdatePortParameters(unsigned long ulPort, tBoolean bSerial,
                           tBoolean bTelnet)
{
    //
    // Do we have to update the telnet settings?  Note that we need to do this
    // first since the act of initiating a telnet connection resets the serial
    // port settings to defaults.
    //
    if(bTelnet)
    {
        //
        // Close any existing connection and shut down the server if required.
        //
        TelnetClose(ulPort);

        //
        // Are we to operate as a telnet server?
        //
        if((g_sParameters.sPort[ulPort].ucFlags & PORT_FLAG_TELNET_MODE) ==
           PORT_TELNET_SERVER)
        {
            //
            // Yes - start listening on the required port.
            //
            TelnetListen(g_sParameters.sPort[ulPort].usTelnetLocalPort,
                         ulPort);
        }
        else
        {
            //
            // No - we are a client so initiate a connection to the desired
            // IP address using the configured ports.
            //
            TelnetOpen(g_sParameters.sPort[ulPort].ulTelnetIPAddr,
                       g_sParameters.sPort[ulPort].usTelnetRemotePort,
                       g_sParameters.sPort[ulPort].usTelnetLocalPort,
                       ulPort);
        }
    }

    //
    // Do we need to update the serial port settings?  We do this if we are
    // told that the serial settings changed or if we just reconfigured the
    // telnet settings (which resets the serial port parameters to defaults as
    // a side effect).
    //
    if(bSerial || bTelnet)
    {
        SerialSetCurrent(ulPort);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs any actions necessary in preparation for a change of IP address.
//!
//! This function is called before ConfigUpdateIPAddress to remove the device
//! from the UPnP network in preparation for a change of IP address or
//! switch between DHCP and StaticIP.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigPreUpdateIPAddress(void)
{
    //
    // Stop UPnP and remove ourselves from the network.
    //
    UPnPStop();
}

//*****************************************************************************
//
//! \internal
//!
//! Sets the IP address selection mode and associated parameters.
//!
//! This function ensures that the IP address selection mode (static IP or
//! DHCP/AutoIP) is set according to the parameters stored in g_sParameters.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateIPAddress(void)
{
    //
    // Change to static/dynamic based on the current settings in the
    // global parameter block.
    //
    if((g_sParameters.ucFlags & CONFIG_FLAG_STATICIP) == CONFIG_FLAG_STATICIP)
    {
        lwIPNetworkConfigChange(g_sParameters.ulStaticIP,
                                g_sParameters.ulSubnetMask,
                                g_sParameters.ulGatewayIP,
                                IPADDR_USE_STATIC);
    }
    else
    {
        lwIPNetworkConfigChange(0, 0, 0, IPADDR_USE_DHCP);
    }

    //
    // Restart UPnP discovery.
    //
    UPnPStart();
}

//*****************************************************************************
//
//! \internal
//!
//! Performs changes as required to apply all active parameters to the system.
//!
//! \param bUpdateIP is set to \e true to update parameters related to the
//! S2E board's IP address. If \e false, the IP address will remain unchanged.
//!
//! This function ensures that the system configuration matches the values in
//! the current, active parameter block.  It is called after the parameter
//! block has been reset to factory defaults.
//!
//! \return None.
//
//*****************************************************************************
void
ConfigUpdateAllParameters(tBoolean bUpdateIP)
{
    //
    // Have we been asked to update the IP address along with the other
    // parameters?
    //
    if(bUpdateIP)
    {
        //
        // Yes - update the IP address selection parameters.
        //
        ConfigPreUpdateIPAddress();
        ConfigUpdateIPAddress();
    }

    //
    // Update the parameters for each of the individual ports.
    //
    ConfigUpdatePortParameters(0, true, true);
    ConfigUpdatePortParameters(1, true, true);

    //
    // Update the name that the board publishes to the locator/finder app.
    //
    LocatorAppTitleSet((char *)g_sParameters.ucModName);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler and returns the
//! index of a given parameter within that list.
//!
//! \param pcToFind is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcToFind.  If the string is found, the index of that string within the
//! \e pcParam array is returned, otherwise -1 is returned.
//!
//! \return Returns the index of string \e pcToFind within array \e pcParam
//! or -1 if the string does not exist in the array.
//
//*****************************************************************************
static int
ConfigFindCGIParameter(const char *pcToFind, char *pcParam[], int iNumParams)
{
    int iLoop;

    //
    // Scan through all the parameters in the array.
    //
    for(iLoop = 0; iLoop < iNumParams; iLoop++)
    {
        //
        // Does the parameter name match the provided string?
        //
        if(strcmp(pcToFind, pcParam[iLoop]) == 0)
        {
            //
            // We found a match - return the index.
            //
            return(iLoop);
        }
    }

    //
    // If we drop out, the parameter was not found.
    //
    return(-1);
}

static tBoolean
ConfigIsValidHexDigit(const char cDigit)
{
    if(((cDigit >= '0') && (cDigit <= '9')) ||
       ((cDigit >= 'a') && (cDigit <= 'f')) ||
       ((cDigit >= 'A') && (cDigit <= 'F')))
    {
        return(true);
    }
    else
    {
        return(false);
    }
}

static unsigned char
ConfigHexDigit(const char cDigit)
{
    if((cDigit >= '0') && (cDigit <= '9'))
    {
        return(cDigit - '0');
    }
    else
    {
        if((cDigit >= 'a') && (cDigit <= 'f'))
        {
            return((cDigit - 'a') + 10);
        }
        else
        {
            if((cDigit >= 'A') && (cDigit <= 'F'))
            {
                return((cDigit - 'A') + 10);
            }
        }
    }

    //
    // If we get here, we were passed an invalid hex digit so return 0xFF.
    //
    return(0xFF);
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a single %xx escape sequence as an ASCII character.
//!
//! \param pcEncoded points to the ``%'' character at the start of a three
//! character escape sequence which represents a single ASCII character.
//! \param pcDecoded points to a byte which will be written with the decoded
//! character assuming the escape sequence is valid.
//!
//! This function decodes a single escape sequence of the form ``%xy'' where
//! x and y represent hexadecimal digits.  If each digit is a valid hex digit,
//! the function writes the decoded character to the pcDecoded buffer and
//! returns true, else it returns false.
//!
//! \return Returns \b true on success or \b false if pcEncoded does not point
//! to a valid escape sequence.
//
//*****************************************************************************
static tBoolean
ConfigDecodeHexEscape(const char *pcEncoded, char *pcDecoded)
{
    if((pcEncoded[0] != '%') || !ConfigIsValidHexDigit(pcEncoded[1]) ||
       !ConfigIsValidHexDigit(pcEncoded[2]))
    {
        return(false);
    }
    else
    {
        *pcDecoded = ((ConfigHexDigit(pcEncoded[1]) * 16) +
                      ConfigHexDigit(pcEncoded[2]));
        return(true);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Encodes a string for use within an HTML tag, escaping non alphanumeric
//! characters.
//!
//! \param pcDecoded is a pointer to a null terminated ASCII string.
//! \param pcEncoded is a pointer to a storage for the encoded string.
//! \param ulLen is the number of bytes of storage pointed to by pcEncoded.
//!
//! This function encodes a string, adding escapes in place of any special,
//! non-alphanumeric characters.  If the encoded string is too long for the
//! provided output buffer, the output will be truncated.
//!
//! \return Returns the number of characters written to the output buffer
//! not including the terminating NULL.
//
//*****************************************************************************
static unsigned long
ConfigEncodeFormString(const char *pcDecoded, char *pcEncoded,
                       unsigned long ulLen)
{
    unsigned long ulLoop;
    unsigned long ulCount;

    //
    // Make sure we were not passed a tiny buffer.
    //
    if(ulLen <= 1)
    {
        return(0);
    }

    //
    // Initialize our output character counter.
    //
    ulCount = 0;

    //
    // Step through each character of the input until we run out of data or
    // space to put our output in.
    //
    for(ulLoop = 0; pcDecoded[ulLoop] && (ulCount < (ulLen - 1)); ulLoop++)
    {
        switch(pcDecoded[ulLoop])
        {
            //
            // Pass most characters without modification.
            //
            default:
            {
                pcEncoded[ulCount++] = pcDecoded[ulLoop];
                break;
            }

            case '\'':
            {
                ulCount += usnprintf(&pcEncoded[ulCount], (ulLen - ulCount),
                                     "&#39;");
                break;
            }
        }
    }

    return(ulCount);
}

//*****************************************************************************
//
//! \internal
//!
//! Decodes a string encoded as part of an HTTP URI.
//!
//! \param pcEncoded is a pointer to a null terminated string encoded as per
//! RFC1738, section 2.2.
//! \param pcDecoded is a pointer to storage for the decoded, null terminated
//! string.
//! \param ulLen is the number of bytes of storage pointed to by pcDecoded.
//!
//! This function decodes a string which has been encoded using the method
//! described in RFC1738, section 2.2 for URLs.  If the decoded string is too
//! long for the provided output buffer, the output will be truncated.
//!
//! \return Returns the number of character written to the output buffer, not
//! including the terminating NULL.
//
//*****************************************************************************
static unsigned long
ConfigDecodeFormString(const  char *pcEncoded, char *pcDecoded,
                       unsigned long ulLen)
{
    unsigned long ulLoop;
    unsigned long ulCount;
    tBoolean bValid;

    ulCount = 0;
    ulLoop = 0;

    //
    // Keep going until we run out of input or fill the output buffer.
    //
    while(pcEncoded[ulLoop] && (ulCount < (ulLen - 1)))
    {
        switch(pcEncoded[ulLoop])
        {
            //
            // '+' in the encoded data is decoded as a space.
            //
            case '+':
            {
                pcDecoded[ulCount++] = ' ';
                ulLoop++;
                break;
            }

            //
            // '%' in the encoded data indicates that the following 2
            // characters indicate the hex ASCII code of the decoded character.
            //
            case '%':
            {
                if(pcEncoded[ulLoop + 1] && pcEncoded[ulLoop + 2])
                {
                    //
                    // Decode the escape sequence.
                    //
                    bValid = ConfigDecodeHexEscape(&pcEncoded[ulLoop],
                                                   &pcDecoded[ulCount]);

                    //
                    // If the escape sequence was valid, move to the next
                    // output character.
                    //
                    if(bValid)
                    {
                        ulCount++;
                    }

                    //
                    // Skip past the escape sequence in the encoded string.
                    //
                    ulLoop += 3;
                }
                else
                {
                    //
                    // We reached the end of the string partway through an
                    // escape sequence so just ignore it and return the number
                    // of decoded characters found so far.
                    //
                    pcDecoded[ulCount] = '\0';
                    return(ulCount);
                }
                break;
            }

            //
            // For all other characters, just copy the input to the output.
            //
            default:
            {
                pcDecoded[ulCount++] = pcEncoded[ulLoop++];
                break;
            }
        }
    }

    //
    // Terminate the string and return the number of characters we decoded.
    //
    pcDecoded[ulCount] = '\0';
    return(ulCount);
}

//*****************************************************************************
//
//! \internal
//!
//! Ensures that a string passed represents a valid decimal number and,
//! if so, converts that number to a long.
//!
//! \param pcValue points to a null terminated string which should contain an
//! ASCII representation of a decimal number.
//! \param plValue points to storage which will receive the number represented
//! by pcValue assuming the string is a valid decimal number.
//!
//! This function determines whether or not a given string represents a valid
//! decimal number and, if it does, converts the string into a decimal number
//! which is returned to the caller.
//!
//! \return Returns \b true if the string is a valid representation of a
//! decimal number or \b false if not.

//*****************************************************************************
static tBoolean
ConfigCheckDecimalParam(const char *pcValue, long *plValue)
{
    unsigned long ulLoop;
    tBoolean bStarted;
    tBoolean bFinished;
    tBoolean bNeg;
    long lAccum;

    //
    // Check that the string is a valid decimal number.
    //
    bStarted = false;
    bFinished = false;
    bNeg = false;
    ulLoop = 0;
    lAccum = 0;

    while(pcValue[ulLoop])
    {
        //
        // Ignore whitespace before the string.
        //
        if(!bStarted)
        {
            if((pcValue[ulLoop] == ' ') || (pcValue[ulLoop] == '\t'))
            {
                ulLoop++;
                continue;
            }

            //
            // Ignore a + or - character as long as we have not started.
            //
            if((pcValue[ulLoop] == '+') || (pcValue[ulLoop] == '-'))
            {
                //
                // If the string starts with a '-', remember to negate the
                // result.
                //
                bNeg = (pcValue[ulLoop] == '-') ? true : false;
                bStarted = true;
                ulLoop++;
            }
            else
            {
                //
                // We found something other than whitespace or a sign character
                // so we start looking for numerals now.
                //
                bStarted = true;
            }
        }

        if(bStarted)
        {
            if(!bFinished)
            {
                //
                // We expect to see nothing other than valid digit characters
                // here.
                //
                if((pcValue[ulLoop] >= '0') && (pcValue[ulLoop] <= '9'))
                {
                    lAccum = (lAccum * 10) + (pcValue[ulLoop] - '0');
                }
                else
                {
                    //
                    // Have we hit whitespace?  If so, check for no more
                    // characters until the end of the string.
                    //
                    if((pcValue[ulLoop] == ' ') || (pcValue[ulLoop] == '\t'))
                    {
                        bFinished = true;
                    }
                    else
                    {
                        //
                        // We got something other than a digit or whitespace so
                        // this makes the string invalid as a decimal number.
                        //
                        return(false);
                    }
                }
            }
            else
            {
                //
                // We are scanning for whitespace until the end of the string.
                //
                if((pcValue[ulLoop] != ' ') && (pcValue[ulLoop] != '\t'))
                {
                    //
                    // We found something other than whitespace so the string
                    // is not valid.
                    //
                    return(false);
                }
            }

            //
            // Move on to the next character in the string.
            //
            ulLoop++;
        }
    }

    //
    // If we drop out of the loop, the string must be valid.  All we need to do
    // now is negate the accumulated value if the string started with a '-'.
    //
    *plValue = bNeg ? -lAccum : lAccum;
    return(true);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for a parameter
//! with the given name and, if found, reads the parameter value as a decimal
//! number.
//!
//! \param pcName is a pointer to a string containing the name of the
//! parameter that is to be found.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find the string passed in
//! \e pcName.  If the string is found, the corresponding parameter value is
//! read from array pcValues and checked to make sure that it is a valid
//! decimal number.  If so, the number is returned.  If any error is detected,
//! parameter \e pbError is written to \b true.  Note that \e pbError is NOT
//! written if the parameter is successfully found and validated.  This is to
//! allow multiple parameters to be parsed without the caller needing to check
//! return codes after each individual call.
//!
//! \return Returns the value of the parameter or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
static long
ConfigGetCGIParam(const char *pcName, char *pcParams[], char *pcValue[],
                  int iNumParams, tBoolean *pbError)
{
    int iParam;
    long lValue;
    tBoolean bRetcode;

    //
    // Is the parameter we are looking for in the list?
    //
    lValue = 0;
    iParam = ConfigFindCGIParameter(pcName, pcParams, iNumParams);
    if(iParam != -1)
    {
        //
        // We found the parameter so now get its value.
        //
        bRetcode = ConfigCheckDecimalParam(pcValue[iParam], &lValue);
        if(bRetcode)
        {
            //
            // All is well - return the parameter value.
            //
            return(lValue);
        }
    }

    //
    // If we reach here, there was a problem so return 0 and set the error
    // flag.
    //
    *pbError = true;
    return(0);
}

//*****************************************************************************
//
//! \internal
//!
//! Searches the list of parameters passed to a CGI handler for 4 parameters
//! representing an IP address and extracts the IP address defined by them.
//!
//! \param pcName is a pointer to a string containing the base name of the IP
//! address parameters.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting the CGI.
//! \param iNumParams is the number of elements in the pcParam array.
//! \param pcValues is an array of values associated with each parameter from
//! the pcParam array.
//! \param pbError is a pointer that will be written to \b true if there is any
//! error during the parameter parsing process (parameter not found, value is
//! not a valid decimal number).
//!
//! This function searches an array of parameters to find four parameters
//! whose names are \e pcName appended with digits 1 - 4.  Each of these
//! parameters is expected to have a value which is a decimal number between
//! 0 and 255.  The parameter values are read and concatenated into an unsigned
//! long representing an IP address with parameter 1 in the leftmost postion.
//!
//! For example, if \e pcName points to string ``ip'', the function will look
//! for 4 CGI parameters named ``ip1'', ``ip2'', ``ip3'' and ``ip4'' and read
//! their values to generate an IP address of the form 0xAABBCCDD where ``AA''
//! is the value of parameter ``ip1'', ``BB'' is the value of ``p2'', ``CC''
//! is the value of ``ip3'' and ``DD'' is the value of ``ip4''.
//!
//! \return Returns the IP address read or 0 if an error is detected (in
//! which case \e *pbError will be \b true).
//
//*****************************************************************************
unsigned long
ConfigGetCGIIPAddr(const char *pcName, char *pcParam[], char *pcValue[],
                   int iNumParams, tBoolean *pbError)
{
    unsigned long ulIPAddr;
    unsigned long ulLoop;
    long lValue;
    char pcVariable[MAX_VARIABLE_NAME_LEN];
    tBoolean bError;

    //
    // Set up for the loop which reads each address element.
    //
    ulIPAddr = 0;
    bError = false;

    //
    // Look for each of the four variables in turn.
    //
    for(ulLoop = 1; ulLoop <= 4; ulLoop++)
    {
        //
        // Generate the name of the variable we are looking for next.
        //
        usnprintf(pcVariable, MAX_VARIABLE_NAME_LEN, "%s%d", pcName, ulLoop);

        //
        // Shift our existing IP address to the left prior to reading the next
        // byte.
        //
        ulIPAddr <<= 8;

        //
        // Get the next variable and mask it into the IP address.
        //
        lValue = ConfigGetCGIParam(pcVariable, pcParam, pcValue, iNumParams,
                                   &bError);
        ulIPAddr |= ((unsigned long)lValue & 0xFF);
    }

    //
    // Did we encounter any error while reading the parameters?
    //
    if(bError)
    {
        //
        // Yes - mark the clients error flag and return 0.
        //
        *pbError = true;
        return(0);
    }
    else
    {
        //
        // No - all is well so return the IP address.
        //
        return(ulIPAddr);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/config.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/config.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``port'' indicates which connection's settings to update.  Valid
//!   values are ``0'' or ``1''.
//! - ``br'' supplies the baud rate.
//! - ``bc'' supplies the number of bits per character.
//! - ``parity'' supplies the parity.  Valid values are ``0'', ``1'', ``2'',
//!   ``3'' or ``4'' with meanings as defined by \b SERIAL_PARITY_xxx in
//!   serial.h.
//! - ``stop'' supplies the number of stop bits.
//! - ``flow'' supplies the flow control setting.  Valid values are ``1'' or
//!   ``3'' with meanings as defined by the \b SERIAL_FLOW_CONTROL_xxx in
//!   serial.h.
//! - ``telnetlp'' supplies the local port number for use by the telnet server.
//! - ``telnetrp'' supplies the remote port number for use by the telnet
//!   client.
//! - ``telnett'' supplies the telnet timeout in seconds.
//! - ``telnetip1'' supplies the first digit of the telnet server IP address.
//! - ``telnetip2'' supplies the second digit of the telnet server IP address.
//! - ``telnetip3'' supplies the third digit of the telnet server IP address.
//! - ``telnetip4'' supplies the fourth digit of the telnet server IP address.
//! - ``tnmode'' supplies the telnet mode, ``0'' for server, ``1'' for client.
//! - ``tnprot'' supplies the telnet protocol, ``0'' for telnet, ``1'' for raw.
//! - ``default'' will be defined with value ``1'' if the settings supplied are
//!   to be saved to flash as the defaults for this port.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static char *
ConfigCGIHandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    long lPort;
    long lValue;
    long lTelnetMode;
    long lTelnetProtocol;
    tBoolean bParamError;
    tBoolean bSerialChanged;
    tBoolean bTelnetChanged;
    tPortParameters sPortParams;

    //
    // We have not encountered any parameter errors yet.
    //
    bParamError = false;

    //
    // Get the port number.
    //
    lPort = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams,
                              &bParamError);

    if(bParamError || ((lPort != 0) && (lPort != 1)))
    {
        //
        // The connection parameter was invalid.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // Take a local copy of the current parameter set for this connection
    //
    sPortParams = g_sParameters.sPort[lPort];

    //
    // Baud rate
    //
    sPortParams.ulBaudRate = (unsigned long)ConfigGetCGIParam("br", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Parity
    //
    sPortParams.ucParity = (unsigned char)ConfigGetCGIParam("parity", pcParam,
                                                            pcValue,
                                                            iNumParams,
                                                            &bParamError);

    //
    // Stop bits
    //
    sPortParams.ucStopBits = (unsigned char)ConfigGetCGIParam("stop", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Data Size
    //
    sPortParams.ucDataSize = (unsigned char)ConfigGetCGIParam("bc", pcParam,
                                                              pcValue,
                                                              iNumParams,
                                                              &bParamError);

    //
    // Flow control
    //
    sPortParams.ucFlowControl = (unsigned char)ConfigGetCGIParam("flow",
                                                                 pcParam,
                                                                 pcValue,
                                                                 iNumParams,
                                                                 &bParamError);

    //
    // Telnet mode
    //
    lTelnetMode = ConfigGetCGIParam("tnmode", pcParam, pcValue, iNumParams,
                                    &bParamError);

    //
    // Telnet protocol
    //
    lTelnetProtocol = ConfigGetCGIParam("tnprot", pcParam, pcValue, iNumParams,
                                    &bParamError);

    //
    // Telnet local port
    //
    sPortParams.usTelnetLocalPort =
        (unsigned short)ConfigGetCGIParam("telnetlp", pcParam, pcValue,
                                         iNumParams, &bParamError);

    //
    // Telnet timeout
    //
    sPortParams.ulTelnetTimeout =
        (unsigned char)ConfigGetCGIParam("telnett", pcParam, pcValue,
                                         iNumParams, &bParamError);

    //
    // If we are in telnet client mode, get the additional parameters required.
    //
    if(lTelnetMode == PORT_TELNET_CLIENT)
    {
        //
        // Telnet remote port
        //
        sPortParams.usTelnetRemotePort =
            (unsigned short)ConfigGetCGIParam("telnetrp", pcParam, pcValue,
                                             iNumParams, &bParamError);

        //
        // Telnet IP address
        //
        sPortParams.ulTelnetIPAddr = ConfigGetCGIIPAddr("telnetip", pcParam,
                                                        pcValue, iNumParams,
                                                        &bParamError);
    }

    //
    // We have now read all the parameters and made sure that they are valid
    // decimal numbers.  Did we see any errors during this process?
    //
    if(bParamError)
    {
        //
        // Yes - tell the user there was an error.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // Update the telnet mode from the parameter we read.
    //
    sPortParams.ucFlags &= ~PORT_FLAG_TELNET_MODE;
    sPortParams.ucFlags |= (lTelnetMode ? PORT_TELNET_CLIENT :
                            PORT_TELNET_SERVER);

    //
    // Update the telnet protocol from the parameter we read.
    //
    sPortParams.ucFlags &= ~PORT_FLAG_PROTOCOL;
    sPortParams.ucFlags |= (lTelnetProtocol ? PORT_PROTOCOL_RAW :
                            PORT_PROTOCOL_TELNET);

    //
    // Did any of the serial parameters change?
    //
    if((g_sParameters.sPort[lPort].ucDataSize != sPortParams.ucDataSize) ||
       (g_sParameters.sPort[lPort].ucFlowControl !=
        sPortParams.ucFlowControl) ||
       (g_sParameters.sPort[lPort].ucParity != sPortParams.ucParity) ||
       (g_sParameters.sPort[lPort].ucStopBits != sPortParams.ucStopBits) ||
       (g_sParameters.sPort[lPort].ulBaudRate != sPortParams.ulBaudRate))
    {
        bSerialChanged = true;
    }
    else
    {
        bSerialChanged = false;
    }

    //
    // Did any of the telnet parameters change?
    //
    if((g_sParameters.sPort[lPort].ulTelnetIPAddr !=
        sPortParams.ulTelnetIPAddr) ||
       (g_sParameters.sPort[lPort].ulTelnetTimeout !=
        sPortParams.ulTelnetTimeout) ||
       (g_sParameters.sPort[lPort].usTelnetLocalPort !=
        sPortParams.usTelnetLocalPort) ||
       (g_sParameters.sPort[lPort].usTelnetRemotePort !=
        sPortParams.usTelnetRemotePort) ||
       ((g_sParameters.sPort[lPort].ucFlags & PORT_FLAG_TELNET_MODE) !=
        (sPortParams.ucFlags & PORT_FLAG_TELNET_MODE)) ||
       ((g_sParameters.sPort[lPort].ucFlags & PORT_FLAG_PROTOCOL) !=
        (sPortParams.ucFlags & PORT_FLAG_PROTOCOL)))
    {
        bTelnetChanged = true;
    }
    else
    {
        bTelnetChanged = false;
    }

    //
    // Update the current parameters with the new settings.
    //
    g_sParameters.sPort[lPort] = sPortParams;

    //
    // Were we asked to save this parameter set as the new default?
    //
    lValue = (unsigned char)ConfigGetCGIParam("default", pcParam, pcValue,
                                              iNumParams, &bParamError);
    if(!bParamError && (lValue == 1))
    {
        //
        // Yes - save these settings as the defaults.
        //
        g_sWorkingDefaultParameters.sPort[lPort] = g_sParameters.sPort[lPort];
        ConfigSave();
    }

    //
    // Apply all the changes to the working parameter set.
    //
    ConfigUpdatePortParameters(lPort, bSerialChanged, bTelnetChanged);

    //
    // Send the user back to the main status page.
    //
    return(DEFAULT_CGI_RESPONSE);
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/ip.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/ip.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``staticip'' contains ``1'' to use a static IP address or ``0'' to use
//!   DHCP/AutoIP.
//! - ``sip1'' contains the first digit of the static IP address.
//! - ``sip2'' contains the second digit of the static IP address.
//! - ``sip3'' contains the third digit of the static IP address.
//! - ``sip4'' contains the fourth digit of the static IP address.
//! - ``gip1'' contains the first digit of the gateway IP address.
//! - ``gip2'' contains the second digit of the gateway IP address.
//! - ``gip3'' contains the third digit of the gateway IP address.
//! - ``gip4'' contains the fourth digit of the gateway IP address.
//! - ``mip1'' contains the first digit of the subnet mask.
//! - ``mip2'' contains the second digit of the subnet mask.
//! - ``mip3'' contains the third digit of the subnet mask.
//! - ``mip4'' contains the fourth digit of the subnet mask.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static char *
ConfigIPCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                   char *pcValue[])
{
    tBoolean bChanged;
    tBoolean bParamError;
    long lMode;
    unsigned long ulIPAddr;
    unsigned long ulGatewayAddr;
    unsigned long ulSubnetMask;

    //
    // Nothing has changed and we have seen no errors so far.
    //
    bChanged = false;
    bParamError = false;
    ulIPAddr = 0;
    ulGatewayAddr = 0;
    ulSubnetMask = 0;

    //
    // Get the IP selection mode.
    //
    lMode = ConfigGetCGIParam("staticip", pcParam, pcValue, iNumParams,
                              &bParamError);

    //
    // This parameter is required so tell the user there has been a problem if
    // it is not found or is invalid.
    //
    if(bParamError)
    {
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // If we are being told to use a static IP, read the remaining information.
    //
    if(lMode)
    {
        //
        // Get the static IP address to use.
        //
        ulIPAddr = ConfigGetCGIIPAddr("sip", pcParam, pcValue, iNumParams,
                                      &bParamError);
        //
        // Get the gateway IP address to use.
        //
        ulGatewayAddr = ConfigGetCGIIPAddr("gip", pcParam, pcValue, iNumParams,
                                           &bParamError);

        ulSubnetMask = ConfigGetCGIIPAddr("mip", pcParam, pcValue, iNumParams,
                                          &bParamError);
    }

    //
    // Make sure we read all the required parameters correctly.
    //
    if(bParamError)
    {
        //
        // Oops - some parameter was invalid.
        //
        return(PARAM_ERROR_RESPONSE);
    }

    //
    // We have all the parameters so determine if anything changed.
    //

    //
    // Did the basic mode change?
    //
    if((lMode && !(g_sParameters.ucFlags & CONFIG_FLAG_STATICIP)) ||
       (!lMode && (g_sParameters.ucFlags & CONFIG_FLAG_STATICIP)))
    {
        //
        // The mode changed so set the new mode in the parameter block.
        //
        if(!lMode)
        {
            g_sParameters.ucFlags &= ~CONFIG_FLAG_STATICIP;
        }
        else
        {
            g_sParameters.ucFlags |= CONFIG_FLAG_STATICIP;
        }

        //
        // Remember that something changed.
        //
        bChanged = true;
    }

    //
    // If we are now using static IP, check for modifications to the IP
    // addresses and mask.
    //
    if(lMode)
    {
        if((g_sParameters.ulStaticIP != ulIPAddr) ||
           (g_sParameters.ulGatewayIP != ulGatewayAddr) ||
           (g_sParameters.ulSubnetMask != ulSubnetMask))
        {
            //
            // Something changed so update the parameter block.
            //
            bChanged = true;
            g_sParameters.ulStaticIP = ulIPAddr;
            g_sParameters.ulGatewayIP = ulGatewayAddr;
            g_sParameters.ulSubnetMask = ulSubnetMask;
        }
    }

    //
    // If anything changed, we need to resave the parameter block.
    //
    if(bChanged)
    {
        //
        // Shut down connections in preparation for the IP address change.
        //
        ConfigPreUpdateIPAddress();

        //
        // Update the working default set and save the parameter block.
        //
        g_sWorkingDefaultParameters = g_sParameters;
        ConfigSave();

        //
        // Tell the main loop that a IP address update has been requested.
        //
        g_cUpdateRequired |= UPDATE_IP_ADDR;

        //
        // Direct the browser to a page warning about the impending IP
        // address change.
        //
        return(IP_UPDATE_RESPONSE);
    }

    //
    // Direct the user back to our miscellaneous settings page.
    //
    return(MISC_PAGE_URI);
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/misc.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/misc.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects the following parameters:
//!
//! - ``modname'' provides a string to be used as the friendly name for the
//!   module.  This is encoded by the browser and must be decoded here.
//! - ``port'' supplies TCP port to be used by UPnP.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static char *
ConfigMiscCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                     char *pcValue[])
{
    int iParam;
    long lValue;
    tBoolean bChanged;
    tBoolean bError;

    //
    // We have not made any changes that need written to flash yet.
    //
    bChanged = false;

    //
    // Find the "modname" parameter.
    //
    iParam = ConfigFindCGIParameter("modname", pcParam, iNumParams);
    if(iParam != -1)
    {
        ConfigDecodeFormString(pcValue[iParam],
                               (char *)g_sWorkingDefaultParameters.ucModName,
                               MOD_NAME_LEN);
        strncpy((char *)g_sParameters.ucModName,
                (char *)g_sWorkingDefaultParameters.ucModName, MOD_NAME_LEN);
        LocatorAppTitleSet((char *)g_sParameters.ucModName);
        bChanged = true;
    }

    //
    // Find the "port" parameter.
    //
    bError = false;
    lValue = ConfigGetCGIParam("port", pcParam, pcValue, iNumParams, &bError);
    if(!bError)
    {
        //
        // The parameter was a valid decimal number.  If it is different
        // from the current value, store it and note that we made a change
        // that needs saving.
        //
        if((unsigned short int)lValue !=
           g_sWorkingDefaultParameters.usLocationURLPort)
        {
            //
            // Shut down UPnP temporarily.
            //
            UPnPStop();

            //
            // Update our working parameters and the default set.
            //
            g_sParameters.usLocationURLPort = (unsigned short)lValue;
            g_sWorkingDefaultParameters.usLocationURLPort =
                (unsigned short int)lValue;

            //
            // Restart UPnP with the new location port number.
            //
            UPnPStart();

            //
            // Remember that something changed.
            //
            bChanged = true;
        }
    }

    //
    // Did anything change?
    //
    if(bChanged)
    {
        //
        // Yes - save the latest parameters to flash.
        //
        ConfigSave();
    }

    return(MISC_PAGE_URI);
}

//*****************************************************************************
//
//! \internal
//!
//! Determines whether the supplied configuration parameter structure indicates
//! that changes are required which are likely to change the board IP address.
//!
//! \param psNow is a pointer to the currently active configuration parameter
//!        structure.
//! \param psNew is a pointer to the configuration parameter structure
//!        containing the latest changes, as yet unapplied.
//!
//! This function is called to determine whether applying a set of
//! configuration parameter changes will required (or likely result in) a change
//! in the local board's IP address.  The function is used to determine whether
//! changes can be made immediately or whether they should be deferred until
//! later, giving the system a chance to send a warning to the user's web
//! browser.
//!
//! \return Returns \e true if an IP address change is likely to occur when
//! the parameters are applied or \e false if the address will not change.
//
//*****************************************************************************
static tBoolean
ConfigWillIPAddrChange(tConfigParameters const *psNow,
                       tConfigParameters const *psNew)
{
    //
    // Did we switch between DHCP/AUTOIP and static IP address?
    //
    if((psNow->ucFlags & CONFIG_FLAG_STATICIP) !=
       (psNew->ucFlags & CONFIG_FLAG_STATICIP))
    {
        //
        // Mode change will almost certainly result in an IP change.
        //
        return(true);
    }

    //
    // If we are using a static IP, check the IP address, subnet mask and
    // gateway address for changes.
    //
    if(psNew->ucFlags & CONFIG_FLAG_STATICIP)
    {
        //
        // Have any of the addresses changed?
        //
        if((psNew->ulStaticIP != psNow->ulStaticIP) ||
           (psNew->ulGatewayIP != psNow->ulGatewayIP) ||
           (psNew->ulSubnetMask != psNow->ulSubnetMask))
        {
            //
            // Yes - either the local IP address or one of the other important
            // IP parameters changed.
            //
            return(true);
        }
    }

    //
    // If we get this far, the IP address, subnet mask or gateway address are
    // not going to change so return false to tell the caller this.
    //
    return(false);
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/defaults.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/defaults.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects no specific parameters and any passed are
//! ignored.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static char *
ConfigDefaultsCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                         char *pcValue[])
{
    tBoolean bAddrChange;

    //
    // Remove us from the UPnP network.
    //
    UPnPStop();

    //
    // Will this update cause an IP address change?
    //
    bAddrChange = ConfigWillIPAddrChange(&g_sParameters, g_psFactoryParameters);

    //
    // Update the working parameter set with the factory defaults.
    //
    ConfigLoadFactory();

    //
    // Save the new defaults to flash.
    //
    ConfigSave();

    //
    // If the IP address won't change, we can apply the other changes
    // immediately.
    //
    if(!bAddrChange)
    {
        //
        // Apply the various changes required as a result of changing back to
        // the default settings.
        //
        ConfigUpdateAllParameters(false);

        //
        // In this case,we send the usual page back to the browser.
        //
        return(DEFAULT_CGI_RESPONSE);
    }
    else
    {
        //
        // The IP address is likely to change so send the browser a warning
        // message and defer the actual update for a couple of seconds by
        // sending a message to the main loop.
        //
        g_cUpdateRequired |= UPDATE_ALL;

        //
        // Send back the warning page.
        //
        return(IP_UPDATE_RESPONSE);
    }
}

//*****************************************************************************
//
//! \internal
//!
//! Performs processing for the URI ``/update.cgi''.
//!
//! \param iIndex is an index into the g_psConfigCGIURIs array indicating which
//! CGI URI has been requested.
//! \param uNumParams is the number of entries in the pcParam and pcValue
//! arrays.
//! \param pcParam is an array of character pointers, each containing the name
//! of a single parameter as encoded in the URI requesting this CGI.
//! \param pcValue is an array of character pointers, each containing the value
//! of a parameter as encoded in the URI requesting this CGI.
//!
//! This function is called whenever the HTTPD server receives a request for
//! URI ``/update.cgi''.  Parameters from the request are parsed into the
//! \e pcParam and \e pcValue arrays such that the parameter name and value
//! are contained in elements with the same index.  The strings contained in
//! \e pcParam and \e pcValue contain all replacements and encodings performed
//! by the browser so the CGI function is responsible for reversing these if
//! required.
//!
//! After processing the parameters, the function returns a fully qualified
//! filename to the HTTPD server which will then open this file and send the
//! contents back to the client in response to the CGI.
//!
//! This specific CGI expects no parameters and ignores all passed.
//!
//! \return Returns a pointer to a string containing the file which is to be
//! sent back to the HTTPD client in response to this request.
//
//*****************************************************************************
static char *
ConfigUpdateCGIHandler(int iIndex, int iNumParams, char *pcParam[],
                       char *pcValue[])
{
    //
    // Remove us from the UPnP network.
    //
    UPnPStop();

    //
    // Set the flag we use to remember that an update has been requested.  Next
    // time we see a "doupdate" SSI tag (which will be placed at the bottom of
    // the response page), we will actually trigger the update flag to the main
    // loop.
    //
    g_bUpdateRequested = true;

    //
    // Send the page confirming that update is about to start.
    //
    return(FIRMWARE_UPDATE_RESPONSE);
}

//*****************************************************************************
//
//! \internal
//!
//! Provides replacement text for each of our configured SSI tags.
//!
//! \param iIndex is an index into the g_pcConfigSSITags array and indicates
//! which tag we are being passed
//! \param pcInsert points to a buffer into which this function should write
//! the replacement text for the tag.  This should be plain text or valid HTML
//! markup.
//! \param iInsertLen is the number of bytes available in the pcInsert buffer.
//! This function must ensure that it does not write more than this or memory
//! corruption will occur.
//!
//! This function is called by the HTTPD server whenever it is serving a page
//! with a ``.ssi'', ``.shtml'' or ``.shtm'' file extension and encounters a
//! tag of the form <!--#tagname--> where ``tagname'' is found in the
//! g_pcConfigSSITags array.  The function writes suitable replacement text to
//! the \e pcInsert buffer.
//!
//! \return Returns the number of bytes written to the pcInsert buffer, not
//! including any terminating NULL.
//
//*****************************************************************************
static int
ConfigSSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    unsigned long ulPort;
    int iCount;
    const char *pcString;

    //
    // Which SSI tag are we being asked to provide content for?
    //
    switch(iIndex)
    {
        //
        // The local IP address tag "ipaddr".
        //
        case SSI_INDEX_IPADDR:
        {
            unsigned long ulIPAddr;

            ulIPAddr = lwIPLocalIPAddrGet();
            return(usnprintf(pcInsert, iInsertLen, "%d.%d.%d.%d",
                             ((ulIPAddr >>  0) & 0xFF),
                             ((ulIPAddr >>  8) & 0xFF),
                             ((ulIPAddr >> 16) & 0xFF),
                             ((ulIPAddr >> 24) & 0xFF)));
        }

        //
        // The local MAC address tag "macaddr".
        //
        case SSI_INDEX_MACADDR:
        {
            unsigned char pucMACAddr[6];

            lwIPLocalMACGet(pucMACAddr);
            return(usnprintf(pcInsert, iInsertLen,
                             "%02X-%02X-%02X-%02X-%02X-%02X", pucMACAddr[0],
                             pucMACAddr[1], pucMACAddr[2], pucMACAddr[3],
                             pucMACAddr[4], pucMACAddr[5]));
        }

        //
        // This tag is used at the bottom of the configuration page which
        // confirms request of a firmware update.  It is used purely as a
        // signal that we can go ahead and trigger the main loop to actually
        // perform the update.  We check that an update has previously been
        // requested before actually initiating it, however, since, if we
        // didn't, any user who stumbled across the URL of the page containing
        // this tag could mess things up nicely.
        //
        case SSI_INDEX_DOUPDATE:
        {
            //
            // If an update was previously requested, signal the main loop to
            // perform it.
            //
            if(g_bUpdateRequested)
            {
                g_bUpdateRequested = false;
                g_bStartBootloader = true;
            }

            //
            // Send back a simple HTML comment.
            //
            return(usnprintf(pcInsert, iInsertLen,
                             "<!-- Update requested -->"));
        }

        //
        // These tag are replaced with the current serial port baud rate for
        // their respective ports.
        //
        case SSI_INDEX_P0BR:
        case SSI_INDEX_P1BR:
        {
            ulPort = (iIndex == SSI_INDEX_P0BR) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetBaudRate(ulPort)));
        }

        //
        // These tag are replaced with the current number of stop bits for
        // the appropriate serial port.
        //
        case SSI_INDEX_P0SB:
        case SSI_INDEX_P1SB:
        {
            ulPort = (iIndex == SSI_INDEX_P0SB) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetStopBits(ulPort)));
        }

        //
        // These tag are replaced with the current parity mode for the
        // appropriate serial port.
        //
        case SSI_INDEX_P0P:
        case SSI_INDEX_P1P:
        {
            ulPort = (iIndex == SSI_INDEX_P0P) ? 0 : 1;
            pcString = ConfigMapIdToString(g_psParityMap, NUM_PARITY_MAPS,
                                           SerialGetParity(ulPort));
            return(usnprintf(pcInsert, iInsertLen, "%s", pcString));
        }

        //
        // These tag are replaced with the current number of bits per character
        // for the appropriate serial port.
        //
        case SSI_INDEX_P0BC:
        case SSI_INDEX_P1BC:
        {
            ulPort = (iIndex == SSI_INDEX_P0BC) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             SerialGetDataSize(ulPort)));
        }

        //
        // These tag are replaced with the current flow control settings for
        // the appropriate serial port.
        //
        case SSI_INDEX_P0FC:
        case SSI_INDEX_P1FC:
        {
            ulPort = (iIndex == SSI_INDEX_P0FC) ? 0 : 1;
            pcString = ConfigMapIdToString(g_psFlowControlMap,
                                           NUM_FLOW_CONTROL_MAPS,
                                           SerialGetFlowControl(ulPort));
            return(usnprintf(pcInsert, iInsertLen, "%s", pcString));
        }

        //
        // These tag are replaced with the timeout for the appropriate
        // port's telnet session.
        //
        case SSI_INDEX_P0TT:
        case SSI_INDEX_P1TT:
        {
            ulPort = (iIndex == SSI_INDEX_P0TT) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.sPort[ulPort].ulTelnetTimeout));
        }

        //
        // These tag are replaced with the local TCP port number in use by the
        // appropriate port's telnet session.
        //
        case SSI_INDEX_P0TLP:
        case SSI_INDEX_P1TLP:
        {
            ulPort = (iIndex == SSI_INDEX_P0TLP) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.sPort[ulPort].usTelnetLocalPort));
        }

        //
        // These tag are replaced with the remote TCP port number in use by
        // the appropriate port's telnet session when in client mode.
        //
        case SSI_INDEX_P0TRP:
        case SSI_INDEX_P1TRP:
        {
            ulPort = (iIndex == SSI_INDEX_P0TRP) ? 0 : 1;
            if((g_sParameters.sPort[ulPort].ucFlags & PORT_FLAG_TELNET_MODE) ==
               PORT_TELNET_SERVER)
            {
                return(usnprintf(pcInsert, iInsertLen, "N/A"));
            }
            else
            {
                return(usnprintf(pcInsert, iInsertLen, "%d",
                              g_sParameters.sPort[ulPort].usTelnetRemotePort));
            }
        }

        //
        // These tag are replaced with a string describing the port's current
        // telnet mode, client or server.
        //
        case SSI_INDEX_P0TNM:
        case SSI_INDEX_P1TNM:
        {
            ulPort = (iIndex == SSI_INDEX_P0TNM) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             ((g_sParameters.sPort[ulPort].ucFlags &
                               PORT_FLAG_TELNET_MODE) == PORT_TELNET_SERVER) ?
                             "Server" : "Client"));
        }

        //
        // These tag are replaced with a string describing the port's current
        // telnet mode, client or server.
        //
        case SSI_INDEX_P0PROT:
        case SSI_INDEX_P1PROT:
        {
            ulPort = (iIndex == SSI_INDEX_P0PROT) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             ((g_sParameters.sPort[ulPort].ucFlags &
                               PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET) ?
                             "Telnet" : "Raw"));
        }

        //
        // These tags are replaced with the full destination IP address for
        // the relevant port's telnet connection (which is only valid
        // when operating as a telnet client).
        //
        case SSI_INDEX_P0TIP:
        case SSI_INDEX_P1TIP:
        {
            ulPort = (iIndex == SSI_INDEX_P0TIP) ? 0 : 1;
            if((g_sParameters.sPort[ulPort].ucFlags & PORT_FLAG_TELNET_MODE) ==
               PORT_TELNET_SERVER)
            {
                return(usnprintf(pcInsert, iInsertLen, "N/A"));
            }
            else
            {
                return(usnprintf(pcInsert, iInsertLen, "%d.%d.%d.%d",
                                 (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                                  24) & 0xFF,
                                 (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                                  16) & 0xFF,
                                 (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                                  8) & 0xFF,
                                 (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                                  0) & 0xFF));
            }
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP1:
        case SSI_INDEX_P1TIP1:
        {
            ulPort = (iIndex == SSI_INDEX_P0TIP1) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                              24) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP2:
        case SSI_INDEX_P1TIP2:
        {
            ulPort = (iIndex == SSI_INDEX_P0TIP2) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                              16) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP3:
        case SSI_INDEX_P1TIP3:
        {
            ulPort = (iIndex == SSI_INDEX_P0TIP3) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                              8) & 0xFF));
        }

        //
        // These tags are replaced with the first (most significant) number
        // in an aa.bb.cc.dd IP address (aa in this case).
        //
        case SSI_INDEX_P0TIP4:
        case SSI_INDEX_P1TIP4:
        {
            ulPort = (iIndex == SSI_INDEX_P0TIP4) ? 0 : 1;
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.sPort[ulPort].ulTelnetIPAddr &
                             0xFF));
        }

        //
        // Generate a block of JavaScript declaring variables which hold the
        // current settings for one of the ports.
        //
        case SSI_INDEX_P0VARS:
        case SSI_INDEX_P1VARS:
        {
            ulPort = (iIndex == SSI_INDEX_P0VARS) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    SER_JAVASCRIPT_VARS,
                                    SerialGetBaudRate(ulPort),
                                    SerialGetStopBits(ulPort),
                                    SerialGetDataSize(ulPort),
                                    SerialGetFlowControl(ulPort),
                                    SerialGetParity(ulPort));
            }
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }

            return(iCount);
        }

        //
        // Return the user-editable friendly name for the module.
        //
        case SSI_INDEX_MODNAME:
        {
            return(usnprintf(pcInsert, iInsertLen, "%s",
                             g_sParameters.ucModName));
        }

        //
        // Return the TCP port number used for UPnP discovery & response.
        //
        case SSI_INDEX_PNPPORT:
        {
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_sParameters.usLocationURLPort));
        }

        //
        // Initialise JavaScript variables containing the information related
        // to the telnet settings for a given port.
        //
        case SSI_INDEX_P0TVARS:
        case SSI_INDEX_P1TVARS:
        {
            ulPort = (iIndex == SSI_INDEX_P0TVARS) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                              TN_JAVASCRIPT_VARS,
                              g_sParameters.sPort[ulPort].ulTelnetTimeout,
                              g_sParameters.sPort[ulPort].usTelnetLocalPort,
                              g_sParameters.sPort[ulPort].usTelnetRemotePort,
                              (((g_sParameters.sPort[ulPort].ucFlags &
                               PORT_FLAG_TELNET_MODE) == PORT_TELNET_SERVER) ?
                                   0 : 1),
                              (((g_sParameters.sPort[ulPort].ucFlags &
                               PORT_FLAG_PROTOCOL) == PORT_PROTOCOL_TELNET) ?
                                   0 : 1));
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Initialise JavaScript variables containing the information related
        // to the telnet settings for a given port.
        //
        case SSI_INDEX_P0IPVAR:
        case SSI_INDEX_P1IPVAR:
        {
            ulPort = (iIndex == SSI_INDEX_P0IPVAR) ? 0 : 1;
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                              TIP_JAVASCRIPT_VARS,
                              (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                               24) & 0xFF,
                              (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                               16) & 0xFF,
                              (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                               8) & 0xFF,
                              (g_sParameters.sPort[ulPort].ulTelnetIPAddr >>
                               0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // static UP address and static/DHCP setting.
        //
        case SSI_INDEX_IPVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    IP_JAVASCRIPT_VARS,
                                    (g_sParameters.ucFlags &
                                     CONFIG_FLAG_STATICIP) ? 1 : 0,
                                    (g_sParameters.ulStaticIP >> 24) & 0xFF,
                                    (g_sParameters.ulStaticIP >> 16) & 0xFF,
                                    (g_sParameters.ulStaticIP >> 8) & 0xFF,
                                    (g_sParameters.ulStaticIP >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // subnet mask.
        //
        case SSI_INDEX_SNVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    SUBNET_JAVASCRIPT_VARS,
                                    (g_sParameters.ulSubnetMask >> 24) & 0xFF,
                                    (g_sParameters.ulSubnetMask >> 16) & 0xFF,
                                    (g_sParameters.ulSubnetMask >> 8) & 0xFF,
                                    (g_sParameters.ulSubnetMask >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate a block of JavaScript variables containing the current
        // subnet mask.
        //
        case SSI_INDEX_GWVARS:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "%s", JAVASCRIPT_HEADER);
            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    GW_JAVASCRIPT_VARS,
                                    (g_sParameters.ulGatewayIP >> 24) & 0xFF,
                                    (g_sParameters.ulGatewayIP >> 16) & 0xFF,
                                    (g_sParameters.ulGatewayIP >> 8) & 0xFF,
                                    (g_sParameters.ulGatewayIP >> 0) & 0xFF);
            }

            if(iCount < iInsertLen)
            {
                iCount += usnprintf(pcInsert + iCount, iInsertLen - iCount,
                                    "%s", JAVASCRIPT_FOOTER);
            }
            return(iCount);
        }

        //
        // Generate an HTML text input field containing the current module
        // name.
        //
        case SSI_INDEX_MODNINP:
        {
            iCount = usnprintf(pcInsert, iInsertLen, "<input value='");

            if(iCount < iInsertLen)
            {
                iCount +=
                    ConfigEncodeFormString((char *)g_sParameters.ucModName,
                                           pcInsert + iCount,
                                           iInsertLen - iCount);
            }

            if(iCount < iInsertLen)
            {
                iCount +=
                    usnprintf(pcInsert + iCount, iInsertLen - iCount,
                              "' maxlength='%d' size='%d' name='modname'>",
                              (MOD_NAME_LEN - 1), MOD_NAME_LEN);
            }
            return(iCount);
        }

        //
        // Generate an HTML text input field containing the current UPnP port
        // number.
        //
        case SSI_INDEX_PNPINP:
        {
            return(usnprintf(pcInsert, iInsertLen,
                             "<input value='%d' maxlength='5' size='6' "
                             "name='port'>", g_sParameters.usLocationURLPort));
        }

        //
        // The Firmware Version number tag, "revision".
        //
        case SSI_INDEX_REVISION:
        {
            return(usnprintf(pcInsert, iInsertLen, "%d",
                             g_usFirmwareVersion));
        }

        //
        // All other tags are unknown.
        //
        default:
        {
            return(usnprintf(pcInsert, iInsertLen,
                             "<b><i>Tag %d unknown!</i></b>", iIndex));
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
