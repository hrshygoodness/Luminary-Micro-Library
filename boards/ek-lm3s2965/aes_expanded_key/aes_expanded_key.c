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
// This is part of revision 8555 of the EK-LM3S2965 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "drivers/rit128x96x4.h"
#include "third_party/aes/aes.h"

//*****************************************************************************
//
//! \addtogroup example_list
//! <h1>AES Pre-expanded Key (aes_expanded_key)</h1>
//!
//! This example shows how to use pre-expanded keys to encrypt some plaintext,
//! and then decrypt it back to the original message.  Using pre-expanded keys
//! avoids the need to perform the expansion at run-time.  This example also
//! uses cipher block chaining (CBC) mode instead of the simpler ECB mode.
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
void AESGenerateIV(unsigned char ucIV[16], int bNewTime);

//*****************************************************************************
//
// Storage for the initialization vector.
//
//*****************************************************************************
unsigned char g_ucIV[16];

//*****************************************************************************
//
// The plain text that will be encrypted.  Note that it is 16 bytes long,
// the size of one block (15 characters plus NULL string terminator).
//
//*****************************************************************************
const char g_cPlainText[16] = "This plain text";

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
    const unsigned *puKey;
    unsigned char ucTempIV[16];

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
    RIT128x96x4StringDraw("AES Expand Example", 12, 8, 15);
    RIT128x96x4StringDraw("------------------", 12, 16, 15);
    RIT128x96x4StringDraw("Plain Text:", 30, 24, 15);
    RIT128x96x4StringDraw(g_cPlainText, 20, 32, 15);

    //
    // Get the expanded key to use for encryption
    //
    puKey = AESExpandedEncryptKeyData();

    //
    // Generate the initialization vector needed for CBC mode.
    // A temporary copy is made that will be used with the crypt
    // function because the crypt function will modify the IV that is passed.
    //
    AESGenerateIV(g_ucIV, 1);
    memcpy(ucTempIV, g_ucIV, 16);

    //
    // Encrypt the plaintext message using CBC mode
    //
    aes_crypt_cbc(puKey, AES_ENCRYPT, 16, ucTempIV,
                  (unsigned char *)g_cPlainText, ucBlockBuf);

    //
    // Print the encrypted block to the display.  Note that it will
    // appear as nonsense data.  The block needs to be null terminated
    // so that the StringDraw function will work correctly.
    //
    ucBlockBuf[16] = 0;
    RIT128x96x4StringDraw("Encrypted:", 34, 48, 15);
    RIT128x96x4StringDraw((char *)ucBlockBuf, 20, 56, 15);

    //
    // Get the expanded key to use for decryption
    //
    puKey = AESExpandedDecryptKeyData();

    //
    // Decrypt the message using CBC mode
    //
    memcpy(ucTempIV, g_ucIV, 16);
    aes_crypt_cbc(puKey, AES_DECRYPT, 16, ucTempIV, ucBlockBuf, ucBlockBuf);

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

