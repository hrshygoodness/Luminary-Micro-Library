//*****************************************************************************
//
// aes_expanded_key.c - Simple example using AES with a pre-expanded key.
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
// This is part of revision 8555 of the EK-LM3S9D92 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "utils/uartstdio.h"
#include "aes/aes.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>AES Pre-expanded Key (aes_expanded_key)</h1>
//!
//! This example shows how to use pre-expanded keys to encrypt some plaintext,
//! and then decrypt it back to the original message.  Using pre-expanded keys
//! avoids the need to perform the expansion at run-time.  This example also
//! uses cipher block chaining (CBC) mode instead of the simpler ECB mode.
//!
//! UART0, connected to the FTDI virtual COM port and running at 115,200,
//! 8-N-1, is used to display messages from this application.
//
//*****************************************************************************

//*****************************************************************************
//
// The following verifies the AES configuration is correct for this example.
//
//*****************************************************************************
#if KEY_FORM != KEY_PRESET
# error This example is for pre-set key use
#endif
#if ENC_VS_DEC != AES_ENC_AND_DEC
# error This example is for encrypt and decrypt
#endif
#if KEY_SIZE != KEYSZ_128 && KEY_SIZE != KEYSZ_ALL
# error This example is for 128-bit key size
#endif
#if !(PROCESSING_MODE & MODE_CBC)
# error This example requires CBC mode
#endif

//*****************************************************************************
//
// These generated header files contain the pre-expanded keys for encryption
// and decryption.
//
//*****************************************************************************
#include "enc_key.h"
#include "dec_key.h"

//*****************************************************************************
//
// Prototype for a function that will generate an initialization vector.
//
//*****************************************************************************
void AESGenerateIV(unsigned char pucIV[16], int bNewTime);

//*****************************************************************************
//
// Storage for the initialization vector.
//
//*****************************************************************************
unsigned char g_pucIV[16];

//*****************************************************************************
//
// The plain text that will be encrypted.  Note that it is 16 bytes long,
// the size of one block (15 characters plus NULL string terminator).
//
//*****************************************************************************
const char g_pcPlainText[16] = "This plain text";

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
// Prints out the contents of a buffer.  First, the buffer bytes are printed
// out in hex, and then in ASCII with non-printable characters replaced with a
// period.
//
//*****************************************************************************
void
PrintBuffer(unsigned char *pucBuffer, unsigned long ulLength)
{
    unsigned long ulIdx;

    //
    // Loop through the characters in the buffer, printing out the hex value.
    //
    for(ulIdx = 0; ulIdx < ulLength; ulIdx++)
    {
        UARTprintf(" %02x", pucBuffer[ulIdx]);
    }

    //
    // Provide a separator between the hex and ASCII version of the buffer.
    //
    UARTprintf("  ");

    //
    // Loop through the characters in the buffer, printing out the ASCII value
    // (replacing non-printable characters with a period).
    //
    for(ulIdx = 0; ulIdx < ulLength; ulIdx++)
    {
        UARTprintf("%c", ((pucBuffer[ulIdx] >= ' ') &&
                          (pucBuffer[ulIdx] <= '~')) ? pucBuffer[ulIdx] : '.');
    }

    //
    // Finish with a newline.
    //
    UARTprintf("\n");
}

//*****************************************************************************
//
// Run the AES encryption/decryption example.
//
//*****************************************************************************
int
main(void)
{
    unsigned char pucBlockBuf[16];
    const unsigned *puKey;
    unsigned char pucTempIV[16];

    //
    // Set the clocking to run directly from the crystal.
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                       SYSCTL_XTAL_16MHZ);

    //
    // Initialize the UART interface.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
    UARTprintf("\033[2JAES encryption/decryption using a pre-expanded key\n");

    //
    // Print the plain text title
    //
    UARTprintf("Plain Text:");
    PrintBuffer((unsigned char *)g_pcPlainText, sizeof(g_pcPlainText));

    //
    // Get the expanded key to use for encryption
    //
    puKey = AESExpandedEncryptKeyData();

    //
    // Generate the initialization vector needed for CBC mode.  A temporary
    // copy is made that will be used with the crypt function because the crypt
    // function will modify the IV that is passed.
    //
    AESGenerateIV(g_pucIV, 1);
    memcpy(pucTempIV, g_pucIV, 16);

    //
    // Encrypt the plaintext message using CBC mode
    //
    aes_crypt_cbc(puKey, AES_ENCRYPT, 16, pucTempIV,
                  (unsigned char *)g_pcPlainText, pucBlockBuf);

    //
    // Print the encrypted block to the display.  Note that it will appear as
    // nonsense data.
    //
    UARTprintf("Encrypted:");
    PrintBuffer(pucBlockBuf, sizeof(pucBlockBuf));

    //
    // Get the expanded key to use for decryption
    //
    puKey = AESExpandedDecryptKeyData();

    //
    // Decrypt the message using CBC mode
    //
    memcpy(pucTempIV, g_pucIV, 16);
    aes_crypt_cbc(puKey, AES_DECRYPT, 16, pucTempIV, pucBlockBuf, pucBlockBuf);

    //
    // Print the decrypted block to the display.  It should be the same text
    // as the original message.
    //
    UARTprintf("Decrypted:");
    PrintBuffer(pucBlockBuf, sizeof(pucBlockBuf));

    //
    // Finished.
    //
    while(1)
    {
    }
}
