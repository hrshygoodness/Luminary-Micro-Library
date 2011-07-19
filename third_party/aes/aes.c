/*
 *  FIPS-197 compliant AES implementation
 *
 *  Based on XySSL: Copyright (C) 2006-2008  Christophe Devine
 *
 *  Copyright (C) 2009  Paul Bakker <polarssl_maintainer at polarssl dot org>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution;
 *      or, the application vendor's website must provide a copy of this
 *      notice.
 *    * Neither the names of PolarSSL or XySSL nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 *  The AES block cipher was designed by Vincent Rijmen and Joan Daemen.
 *
 *  http://csrc.nist.gov/encryption/aes/rijndael/Rijndael.pdf
 *  http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
 */
/*
 * This file was taken from PolarSSL-0.10.1 and modified by Texas Instruments.
 */

// the next are just dummy defines to allow left column marking of
// chunks of code which are removed by #if, so you can tell what is
// there or not easily
#define E               // if encrypt
#define ES              // if encrypt or key-set
#define EA              // encrypt and all tables
#define D               // if decrypt
#define DA              // descrypt and all tables
#define K               // if key-set (vs. pre-set)
#define KD              // if key-set and decrypt
#define C               // CBC
#define CE              // CBC and encrypt
#define CD              // CBC and decrypt
#define F               // CFB
#define FE              // CFB and encrypt
#define FD              // CFB and decrypt

#include <string.h>

// include header for interface, including configuration
#include "aes.h"

// now define the tables as needed
#if TABLE_IN_ROM
#include "driverlib/rom.h"

//
// Structure that defines the layout of the tables in ROM
//
struct AES_TABLES
{
    unsigned char   fsb[256];   // Forward S-box
    unsigned        ftab[256];  // Forward table
    unsigned char   rsb[256];   // Reverse S-box
    unsigned        rtab[256];  // Reverse table
};

//
// Macro to create a pointer for access to the tables in ROM
//
#define AES_IN_ROM ((const struct AES_TABLES *)ROM_pvAESTable)

#endif

#include "aes_table_defs.h"     // AES table setup

/*
 * Round constants
 */
#if KEY_FORM == KEY_SET
static const unsigned RCON[10] =
{
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x0000001B, 0x00000036
};
#endif

#if KEY_FORM == KEY_SET
/*
 * AES key schedule (encryption)
 */
void aes_setkey_enc( aes_context *ctx, const unsigned char *key, int keysize )
{
K   int i;
K   unsigned *RK;
K
K   switch( keysize )
K   {
K       case 128: NUM_ROUNDS = NUM_ROUNDS_128; break;
K       case 192: NUM_ROUNDS = NUM_ROUNDS_192; break;
K       case 256: NUM_ROUNDS = NUM_ROUNDS_256; break;
K       default : return;
K   }
K
K   RK = ctx->buf;
K
K   for( i = 0; i < (keysize >> 5); i++ )
K   {
K     RK[i] = ((unsigned *)key)[i];
K   }
K
K   switch( NUM_ROUNDS )
K   {
#       if KEY_SIZE==KEYSZ_128||KEY_SIZE==KEYSZ_ALL
K       case NUM_ROUNDS_128:
K           for( i = 0; i < 10; i++, RK += 4 )
K           {
K               RK[4]  = RK[0] ^ RCON[i] ^
K                   ( FSb[ ( RK[3] >>  8 ) & 0xFF ]       ) ^
K                   ( FSb[ ( RK[3] >> 16 ) & 0xFF ] <<  8 ) ^
K                   ( FSb[ ( RK[3] >> 24 ) & 0xFF ] << 16 ) ^
K                   ( FSb[ ( RK[3]       ) & 0xFF ] << 24 );
K
K               RK[5]  = RK[1] ^ RK[4];
K               RK[6]  = RK[2] ^ RK[5];
K               RK[7]  = RK[3] ^ RK[6];
K           }
K           break;
#           endif
#       if KEY_SIZE==KEYSZ_192||KEY_SIZE==KEYSZ_ALL
K       case NUM_ROUNDS_192:
K           for( i = 0; i < 8; i++, RK += 6 )
K           {
K               RK[6]  = RK[0] ^ RCON[i] ^
K                   ( FSb[ ( RK[5] >>  8 ) & 0xFF ]       ) ^
K                   ( FSb[ ( RK[5] >> 16 ) & 0xFF ] <<  8 ) ^
K                   ( FSb[ ( RK[5] >> 24 ) & 0xFF ] << 16 ) ^
K                   ( FSb[ ( RK[5]       ) & 0xFF ] << 24 );
K
K               RK[7]  = RK[1] ^ RK[6];
K               RK[8]  = RK[2] ^ RK[7];
K               RK[9]  = RK[3] ^ RK[8];
K               RK[10] = RK[4] ^ RK[9];
K               RK[11] = RK[5] ^ RK[10];
K           }
K           break;
#           endif
#       if KEY_SIZE==KEYSZ_256||KEY_SIZE==KEYSZ_ALL
K       case NUM_ROUNDS_256:
K           for( i = 0; i < 7; i++, RK += 8 )
K           {
K               RK[8]  = RK[0] ^ RCON[i] ^
K                   ( FSb[ ( RK[7] >>  8 ) & 0xFF ]       ) ^
K                   ( FSb[ ( RK[7] >> 16 ) & 0xFF ] <<  8 ) ^
K                   ( FSb[ ( RK[7] >> 24 ) & 0xFF ] << 16 ) ^
K                   ( FSb[ ( RK[7]       ) & 0xFF ] << 24 );
K
K               RK[9]  = RK[1] ^ RK[8];
K               RK[10] = RK[2] ^ RK[9];
K               RK[11] = RK[3] ^ RK[10];
K
K               RK[12] = RK[4] ^
K                   ( FSb[ ( RK[11]       ) & 0xFF ]       ) ^
K                   ( FSb[ ( RK[11] >>  8 ) & 0xFF ] <<  8 ) ^
K                   ( FSb[ ( RK[11] >> 16 ) & 0xFF ] << 16 ) ^
K                   ( FSb[ ( RK[11] >> 24 ) & 0xFF ] << 24 );
K
K               RK[13] = RK[5] ^ RK[12];
K               RK[14] = RK[6] ^ RK[13];
K               RK[15] = RK[7] ^ RK[14];
K           }
K           break;
#           endif
K       default:
K           break;
K   }
}

#if (KEY_FORM == KEY_SET) && (ENC_VS_DEC&AES_DEC)
/*
 * AES key schedule (decryption)
 */
void aes_setkey_dec( aes_context *ctx, const unsigned char *key, int keysize )
{
KD  int i, j;
KD  aes_context cty;
KD  unsigned *RK;
KD  unsigned *SK;
KD
KD  switch( keysize )
KD  {
KD      case 128: NUM_ROUNDS = NUM_ROUNDS_128; break;
KD      case 192: NUM_ROUNDS = NUM_ROUNDS_192; break;
KD      case 256: NUM_ROUNDS = NUM_ROUNDS_256; break;
KD      default : return;
KD  }
KD
KD  RK = ctx->buf;
KD
KD  aes_setkey_enc( &cty, key, keysize );
KD  SK = cty.buf + cty.nr * 4;
KD
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD
KD  for( i = NUM_ROUNDS, SK -= 8; i > 1; i--, SK -= 8 )
KD  {
KD      for( j = 0; j < 4; j++, SK++ )
KD      {
KD          *RK++ = _RT0( FSb[ ( *SK       ) & 0xFF ] ) ^
KD                  _RT1( FSb[ ( *SK >>  8 ) & 0xFF ] ) ^
KD                  _RT2( FSb[ ( *SK >> 16 ) & 0xFF ] ) ^
KD                  _RT3( FSb[ ( *SK >> 24 ) & 0xFF ] );
KD      }
KD  }
KD
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD  *RK++ = *SK++;
KD
KD  //memset( &cty, 0, sizeof( aes_context ) );
}
#endif                          // ENC_VS_DEC&AES_DEC
#endif                          // KEY_FORM == KEY_SET

#define AES_FROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
    X0 = *RK++ ^ _FT0( ( Y0       ) & 0xFF ) ^   \
                 _FT1( ( Y1 >>  8 ) & 0xFF ) ^   \
                 _FT2( ( Y2 >> 16 ) & 0xFF ) ^   \
                 _FT3( ( Y3 >> 24 ) & 0xFF );    \
                                                \
    X1 = *RK++ ^ _FT0( ( Y1       ) & 0xFF ) ^   \
                 _FT1( ( Y2 >>  8 ) & 0xFF ) ^   \
                 _FT2( ( Y3 >> 16 ) & 0xFF ) ^   \
                 _FT3( ( Y0 >> 24 ) & 0xFF );    \
                                                \
    X2 = *RK++ ^ _FT0( ( Y2       ) & 0xFF ) ^   \
                 _FT1( ( Y3 >>  8 ) & 0xFF ) ^   \
                 _FT2( ( Y0 >> 16 ) & 0xFF ) ^   \
                 _FT3( ( Y1 >> 24 ) & 0xFF );    \
                                                \
    X3 = *RK++ ^ _FT0( ( Y3       ) & 0xFF ) ^   \
                 _FT1( ( Y0 >>  8 ) & 0xFF ) ^   \
                 _FT2( ( Y1 >> 16 ) & 0xFF ) ^   \
                 _FT3( ( Y2 >> 24 ) & 0xFF );    \
}

#define AES_RROUND(X0,X1,X2,X3,Y0,Y1,Y2,Y3)     \
{                                               \
    X0 = *RK++ ^ _RT0( ( Y0       ) & 0xFF ) ^   \
                 _RT1( ( Y3 >>  8 ) & 0xFF ) ^   \
                 _RT2( ( Y2 >> 16 ) & 0xFF ) ^   \
                 _RT3( ( Y1 >> 24 ) & 0xFF );    \
                                                \
    X1 = *RK++ ^ _RT0( ( Y1       ) & 0xFF ) ^   \
                 _RT1( ( Y0 >>  8 ) & 0xFF ) ^   \
                 _RT2( ( Y3 >> 16 ) & 0xFF ) ^   \
                 _RT3( ( Y2 >> 24 ) & 0xFF );    \
                                                \
    X2 = *RK++ ^ _RT0( ( Y2       ) & 0xFF ) ^   \
                 _RT1( ( Y1 >>  8 ) & 0xFF ) ^   \
                 _RT2( ( Y0 >> 16 ) & 0xFF ) ^   \
                 _RT3( ( Y3 >> 24 ) & 0xFF );    \
                                                \
    X3 = *RK++ ^ _RT0( ( Y3       ) & 0xFF ) ^   \
                 _RT1( ( Y2 >>  8 ) & 0xFF ) ^   \
                 _RT2( ( Y1 >> 16 ) & 0xFF ) ^   \
                 _RT3( ( Y0 >> 24 ) & 0xFF );    \
}

/*
 * AES-ECB block encryption/decryption
 * ECB (Electronic Code Book) is non-chained (each block is separately
 * encrypted). This does not need an init vector (IV).
 */
void aes_crypt_ecb(ECB_CONTEXT, int mode,
                const unsigned char input[16], unsigned char output[16])
{
    int i;
    unsigned X0, X1, X2, X3, Y0, Y1, Y2, Y3;
    const unsigned *RK;

    RK = ROUND_KEYS;

    X0 = ((unsigned*)input)[0]; X0 ^= *RK++;
    X1 = ((unsigned*)input)[1]; X1 ^= *RK++;
    X2 = ((unsigned*)input)[2]; X2 ^= *RK++;
    X3 = ((unsigned*)input)[3]; X3 ^= *RK++;

#   if ENC_VS_DEC==AES_ENC_AND_DEC
    if( mode == AES_ENCRYPT )
#   endif
#   if ENC_VS_DEC&AES_ENC
E   {
E       for( i = (NUM_ROUNDS >> 1); i > 1; i-- )
E       {
E           AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
E           AES_FROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
E       }
E
E       AES_FROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
E
E       X0 = *RK++ ^ ( FSb[ ( Y0       ) & 0xFF ]       ) ^
E                    ( FSb[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
E                    ( FSb[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
E                    ( FSb[ ( Y3 >> 24 ) & 0xFF ] << 24 );
E
E       X1 = *RK++ ^ ( FSb[ ( Y1       ) & 0xFF ]       ) ^
E                    ( FSb[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
E                    ( FSb[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
E                    ( FSb[ ( Y0 >> 24 ) & 0xFF ] << 24 );
E
E       X2 = *RK++ ^ ( FSb[ ( Y2       ) & 0xFF ]       ) ^
E                    ( FSb[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
E                    ( FSb[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
E                    ( FSb[ ( Y1 >> 24 ) & 0xFF ] << 24 );
E
E       X3 = *RK++ ^ ( FSb[ ( Y3       ) & 0xFF ]       ) ^
E                    ( FSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
E                    ( FSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
E                    ( FSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );
E   }
#   endif                       // AES_ENC
#   if ENC_VS_DEC==AES_ENC_AND_DEC
    else /* AES_DECRYPT */
#   endif
#   if ENC_VS_DEC&AES_DEC
D   {
D       for( i = (NUM_ROUNDS >> 1); i > 1; i-- )
D       {
D           AES_RROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
D           AES_RROUND( X0, X1, X2, X3, Y0, Y1, Y2, Y3 );
D       }
D
D       AES_RROUND( Y0, Y1, Y2, Y3, X0, X1, X2, X3 );
D
D       X0 = *RK++ ^ ( RSb[ ( Y0       ) & 0xFF ]       ) ^
D                    ( RSb[ ( Y3 >>  8 ) & 0xFF ] <<  8 ) ^
D                    ( RSb[ ( Y2 >> 16 ) & 0xFF ] << 16 ) ^
D                    ( RSb[ ( Y1 >> 24 ) & 0xFF ] << 24 );
D
D       X1 = *RK++ ^ ( RSb[ ( Y1       ) & 0xFF ]       ) ^
D                    ( RSb[ ( Y0 >>  8 ) & 0xFF ] <<  8 ) ^
D                    ( RSb[ ( Y3 >> 16 ) & 0xFF ] << 16 ) ^
D                    ( RSb[ ( Y2 >> 24 ) & 0xFF ] << 24 );
D
D       X2 = *RK++ ^ ( RSb[ ( Y2       ) & 0xFF ]       ) ^
D                    ( RSb[ ( Y1 >>  8 ) & 0xFF ] <<  8 ) ^
D                    ( RSb[ ( Y0 >> 16 ) & 0xFF ] << 16 ) ^
D                    ( RSb[ ( Y3 >> 24 ) & 0xFF ] << 24 );
D
D       X3 = *RK++ ^ ( RSb[ ( Y3       ) & 0xFF ]       ) ^
D                    ( RSb[ ( Y2 >>  8 ) & 0xFF ] <<  8 ) ^
D                    ( RSb[ ( Y1 >> 16 ) & 0xFF ] << 16 ) ^
D                    ( RSb[ ( Y0 >> 24 ) & 0xFF ] << 24 );
D   }
#   endif                       // AES_DEC

    ((unsigned*)output)[0] = X0;
    ((unsigned*)output)[1] = X1;
    ((unsigned*)output)[2] = X2;
    ((unsigned*)output)[3] = X3;
}


/*
 * AES-CBC buffer encryption/decryption
 * CBC (Cipher Block Chaining) is chained in that it XORs the
 * preceding block's encrypted text with the plaintext (and an
 * init vector (IV) is used for the 1st).
 */
#if PROCESSING_MODE&MODE_CBC
void aes_crypt_cbc(ECB_CONTEXT, int mode, int length, unsigned char iv[16],
                const unsigned char *input, unsigned char *output)
{
C   int i;
C   unsigned char temp[16];
C
#   if ENC_VS_DEC==AES_ENC_AND_DEC
C   if( mode == AES_ENCRYPT )
#   endif
#   if ENC_VS_DEC&AES_ENC
CE  {
CE      while( length > 0 )
CE      {
CE          for( i = 0; i < 16; i++ )
CE              output[i] = (unsigned char)( input[i] ^ iv[i] );
CE
CE          aes_crypt_ecb( ECB_ARG, mode, output, output );
CE          memcpy( iv, output, 16 );
CE
CE          input  += 16;
CE          output += 16;
CE          length -= 16;
CE      }
CE  }
#   endif                       // AES_ENC
#   if ENC_VS_DEC==AES_ENC_AND_DEC
C   else /* AES_DECRYPT */
#   endif
#   if ENC_VS_DEC&AES_DEC
CD  {
CD      while( length > 0 )
CD      {
CD          memcpy( temp, input, 16 );
CD          aes_crypt_ecb( ECB_ARG, mode, input, output );
CD
CD          for( i = 0; i < 16; i++ )
CD              output[i] = (unsigned char)( output[i] ^ iv[i] );
CD
CD          memcpy( iv, temp, 16 );
CD
CD          input  += 16;
CD          output += 16;
CD          length -= 16;
CD      }
CD  }
#   endif                       // AES_DEC
}
#endif                          // PROCESSING_MODE&MODE_CBC

#if PROCESSING_MODE&MODE_CFB
/*
 * AES-CFB buffer encryption/decryption
 * CFB (Cipher FeedBack) is like CBC except the plaintext is XORed
 * with the result, which then forms both the result and the input
 * to the next. The IV is used for the 1st.
 */
void aes_crypt_cfb(ECB_CONTEXT, int mode, int length, int *iv_off, unsigned char iv[16],
                const unsigned char *input, unsigned char *output)
{
F   int c, n = *iv_off;
F
#   if ENC_VS_DEC==AES_ENC_AND_DEC
F   if( mode == AES_ENCRYPT )
#   endif
#   if ENC_VS_DEC&AES_ENC
FE  {
FE      while( length-- )
FE      {
FE          if( n == 0 )
FE              aes_crypt_ecb( ECB_ARG, AES_ENCRYPT, iv, iv );
FE
FE          iv[n] = *output++ = (unsigned char)( iv[n] ^ *input++ );
FE
FE          n = (n + 1) & 0x0F;
FE      }
FE  }
#   endif                       // AES_ENC
#   if ENC_VS_DEC==AES_ENC_AND_DEC
F   else                        /* AES_DECRYPT */
#   endif
#   if ENC_VS_DEC&AES_DEC
FD  {
FD      while( length-- )
FD      {
FD          if( n == 0 )
FD              aes_crypt_ecb( ECB_ARG, AES_ENCRYPT, iv, iv );
FD
FD          c = *input++;
FD          *output++ = (unsigned char)( c ^ iv[n] );
FD          iv[n] = (unsigned char) c;
FD
FD          n = (n + 1) & 0x0F;
FD      }
FD  }
#   endif                       // AES_DEC
F
F   *iv_off = n;
}
#endif                          // PROCESSING_MODE&MODE_CFB

#if PROCESSING_MODE&MODE_CTR
/*
 * AES-CTR buffer encryption/decryption
 * CTR (Counter) uses an IV which includes the block position and
 * is itself encrypted, and then uses XOR of the plaintext (encrypt)
 * or cipher (decrypt) to form the result. This can be encrypted or
 * decrypted in any order and blocks can be skipped.
 * Note that the IV/nonce must be very unique or else an attack can
 * easily decrypt (or encrypt fake messages).
 */
void aes_crypt_ctr(ECB_CONTEXT, int mode, int length, unsigned char iv[16],
                const unsigned char *input, unsigned char *output,
                unsigned short *count)
{
F  int c, n = 0;
F  unsigned short cnt = *count;
F  // Note: this is symmetrical for enc/dec (except input as plain/cipher)
F  while( length-- )
F  {
F    if( n == 0 )
F    {
F       iv[14] = (unsigned char)(cnt>>8);
F       iv[15] = (unsigned char)cnt;
F       cnt++;
F       aes_crypt_ecb( ECB_ARG, mode, iv, output );
F    }
F
F    *output++ ^= *input++;
F
F    n = (n + 1) & 0x0F;
F  }
F  *count = cnt;
}
#endif                                                  // PROCESSING_MODE&MODE_CTR



