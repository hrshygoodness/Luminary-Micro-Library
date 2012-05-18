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
// This is part of revision 8555 of the EK-LM3S6965 Firmware Package.
//
//*****************************************************************************

#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "third_party/aes/aes.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>AES Normal Key (aes_set_key)</h1>
//!
//! This example shows how to set an encryption key and then use that key to
//! encrypt some plaintext.  It then sets the decryption key and decrypts the
//! previously encrypted block back to plaintext.
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
const unsigned char g_ucKey[16] =
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
const char g_cPlainText[16] = "This plain text";

//*****************************************************************************
//
// The context structure for the AES functions.  This structure contains a
// buffer so it is best to not put it on the stack unless you make the stack
// larger.
//
//*****************************************************************************
static aes_context g_aesCtx;

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
// Run the AES encryption/decryption example
//
//*****************************************************************************
int
main(void)
{
    unsigned char ucBlockBuf[17];

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);

    //
    // Initialize the OLED display.
    //
    RIT128x96x4Init(1000000);

    //
    // Print a title and the plain text to the display
    //
    RIT128x96x4StringDraw("AES Key Example", 20, 8, 15);
    RIT128x96x4StringDraw("---------------", 20, 16, 15);
    RIT128x96x4StringDraw("Plain Text:", 30, 24, 15);
    RIT128x96x4StringDraw(g_cPlainText, 20, 32, 15);

    //
    // Set the key to use for encryption
    //
    aes_setkey_enc(&g_aesCtx, g_ucKey, 128);

    //
    // Encrypt the plaintext message using ECB mode
    //
    aes_crypt_ecb(&g_aesCtx, AES_ENCRYPT, (unsigned char*)g_cPlainText,
                  ucBlockBuf);

    //
    // Print the encrypted block to the display.  Note that it will
    // appear as nonsense data.  The block needs to be null terminated
    // so that the StringDraw function will work correctly.
    //
    ucBlockBuf[16] = 0;
    RIT128x96x4StringDraw("Encrypted:", 34, 48, 15);
    RIT128x96x4StringDraw((char *)ucBlockBuf, 20, 56, 15);

    //
    // Set the key to use for decryption
    //
    aes_setkey_dec(&g_aesCtx, g_ucKey, 128);

    //
    // Decrypt the message
    //
    aes_crypt_ecb(&g_aesCtx, AES_DECRYPT, ucBlockBuf, ucBlockBuf);

    //
    // Print the decrypted block to the display.  It should be the same text
    // as the original message.
    //
    RIT128x96x4StringDraw("Decrypted:", 34, 72, 15);
    RIT128x96x4StringDraw((char *)ucBlockBuf, 20, 80, 15);

    //
    // Finished.
    //
    while(1)
    {
    }
}

