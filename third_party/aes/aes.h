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

/**
 * \file aes.h
 */
#ifndef XYSSL_AES_H
#define XYSSL_AES_H

// configuration is important
#include "aes_config_opts.h"
#include "aes_config.h"

#define AES_ENCRYPT     0
#define AES_DECRYPT     1

#if KEY_FORM == KEY_SET
# define ROUND_KEYS     ctx->buf // expanded key
# define NUM_ROUNDS     ctx->nr // number of rounds
# define ECB_CONTEXT    aes_context *ctx
# define ECB_ARG        ctx
/**
 * \brief          AES context structure, not used for pre-set keys
 */
typedef struct
{
    int nr;                     /*!<  number of rounds  */
    unsigned buf[AESCONSZ];     /*!<  key after processing for rounds    */
} aes_context;
#else
# define ROUND_KEYS     key_expand
# define NUM_ROUNDS     FIXED_NUM_ROUNDS
# define ECB_CONTEXT    const unsigned *key_expand
# define ECB_ARG        key_expand
#endif


#ifdef __cplusplus
extern "C" {
#endif

#if KEY_FORM == KEY_SET
/**
 * \brief          AES key schedule (encryption) - used when not pre-set keys
 *
 * \param ctx      AES context to be initialized
 * \param key      encryption key
 * \param keysize  must be 128, 192 or 256
 */
void aes_setkey_enc( aes_context *ctx, const unsigned char *key, int keysize );

/**
 * \brief          AES key schedule (decryption) - used when not pre-set keys
 *
 * \param ctx      AES context to be initialized
 * \param key      decryption key
 * \param keysize  must be 128, 192 or 256
 */
void aes_setkey_dec( aes_context *ctx, const unsigned char *key, int keysize );

#endif                          // KEY_FORM != KEY_PRESET

/**
 * \brief          AES-ECB block encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 */
void aes_crypt_ecb( ECB_CONTEXT,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] );

#if PROCESSING_MODE&MODE_CBC
/**
 * \brief          AES-CBC buffer encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
void aes_crypt_cbc( ECB_CONTEXT,
                    int mode,
                    int length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );
#endif                  // MODE_CBC

#if PROCESSING_MODE&MODE_CFB
/**
 * \brief          AES-CFB buffer encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */
void aes_crypt_cfb( ECB_CONTEXT,
                    int mode,
                    int length,
                    int *iv_off,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );
#endif                  // MODE_CFB

#if PROCESSING_MODE&MODE_CTR
/**
 * \brief          AES-CTR buffer encryption/decryption
 *
 * \param ctx      AES context
 * \param mode     AES_ENCRYPT or AES_DECRYPT
 * \param length   length of the input data (must be multiple of 16)
 * \param iv       initialization vector (bottom two bytes modified)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * \param count    current count to use (for counter), modified after
 */
void aes_crypt_ctr( ECB_CONTEXT,
                    int mode,
                    int length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output,
                    unsigned short *count );
#endif                  // MODE_CTR


/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int aes_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* aes.h */
