//*****************************************************************************
//
// aes_set_key.c - Simple example using AES with a normal key.
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
// This is part of revision 8555 of the EK-LM3S9B90 Firmware Package.
//
//*****************************************************************************

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
//! <h1>AES Normal Key (aes_set_key)</h1>
//!
//! This example shows how to set an encryption key and then use that key to
//! encrypt some plaintext.  It then sets the decryption key and decrypts the
//! previously encrypted block back to plaintext.
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
#if KEY_FORM != KEY_SET
# error This example is for normal key encoding use
#endif
#if ENC_VS_DEC != AES_ENC_AND_DEC
# error This example is for encrypt and decrypt
#endif
#if KEY_SIZE != KEYSZ_128 && KEY_SIZE != KEYSZ_ALL
# error This example is for 128-bit key size
#endif

//*****************************************************************************
//
// The key to use for encryption.  Note that this key is not a good example
// since it is not random.
//
//*****************************************************************************
const unsigned char g_pucKey[16] =
{
    0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89,
    0x9A, 0xAB, 0xBC, 0xCD, 0xDE, 0xEF, 0xF0, 0x00
};

//*****************************************************************************
//
// The plain text that will be encrypted.  Note that it is 16 bytes long,
// the size of one block (15 characters plus NULL string terminator).
//
//*****************************************************************************
const char g_pcPlainText[16] = "This plain text";

//*****************************************************************************
//
// The context structure for the AES functions.  This structure contains a
// buffer so it is best to not put it on the stack unless you make the stack
// larger.
//
//*****************************************************************************
static aes_context g_sAESCtx;

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
    UARTprintf("\033[2JAES encryption/decryption using a normal key\n");

    //
    // Print the plain text title
    //
    UARTprintf("Plain Text:");
    PrintBuffer((unsigned char *)g_pcPlainText, sizeof(g_pcPlainText));

    //
    // Set the key to use for encryption
    //
    aes_setkey_enc(&g_sAESCtx, g_pucKey, 128);

    //
    // Encrypt the plaintext message using ECB mode
    //
    aes_crypt_ecb(&g_sAESCtx, AES_ENCRYPT, (unsigned char *)g_pcPlainText,
                  pucBlockBuf);

    //
    // Print the encrypted block to the display.  Note that it will
    // appear as nonsense data.
    //
    UARTprintf("Encrypted:");
    PrintBuffer(pucBlockBuf, sizeof(pucBlockBuf));

    //
    // Set the key to use for decryption
    //
    aes_setkey_dec(&g_sAESCtx, g_pucKey, 128);

    //
    // Decrypt the message
    //
    aes_crypt_ecb(&g_sAESCtx, AES_DECRYPT, pucBlockBuf, pucBlockBuf);

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
