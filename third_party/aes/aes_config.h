/*
 * This contents of this file were derived from AES code that was taken
 * from PolarSSL-0.10.1 and modified by Texas Instruments.
 */

//----- Choose encode, decode, or both --------------
// Default is both
#define AES_ENC         1
#define AES_DEC         2
#define AES_ENC_AND_DEC 3
#ifndef ENC_VS_DEC
# define ENC_VS_DEC      AES_ENC_AND_DEC
#endif
//----- Choose between 1 table and 4 tables ---------
// Default is 1 table
//
// Tables take up more Flash space. 1 table is a bit
// slower due to extra extracts from single table
#define ONE_TABLE       1
#define ALL_TABLES      2
#ifndef TABLE_MODEL
# define TABLE_MODEL     ONE_TABLE
#endif
//----- Choose between ECB, CBC, CFB, CTR ------------
// Default: CBC only
//
// ECB (Electronic CodeBook) processes each block separately
// CBC (Chained Block Cipher) carries results from one block to next
// CFB (Cipher FeedBack) carries results from one block to next
// CTR (Counter) uses a block position counter to allow random access.
//
// OR-mask of 1 or more methods. Note that ECB is included no
// matter what since it is the basis of all encryption.
#define MODE_ECB        (1<<0)
#define MODE_CBC        (1<<1)
#define MODE_CFB        (1<<2)
#define MODE_CTR        (1<<3)
#ifndef PROCESSING_MODE
# define PROCESSING_MODE MODE_ECB
#endif
//----- Choose between set-key and pre-set -------------
// Default: Set-Key
//
// Pre-set means stored in memory. KEY_PRESET_CODE is
// safer since it stores the values as code, which can be
// OTPed as execute-only, making read out attacks harder.
#define KEY_SET         1
#define KEY_PRESET      2       // preset expanded as aes_key_enc and aes_key_dec
#define KEY_PRESET_CODE 3               // preset, but stored as instrs only
#ifndef KEY_FORM
# define KEY_FORM        KEY_SET
#endif
//----- Choose allowed key sizes ------------------------
// Default: 128 only
//
// The size of key can be fixed at once size, or you can
// allow the call to pass in the size.
#define KEYSZ_128       128
#define KEYSZ_192       192
#define KEYSZ_256       256
#define KEYSZ_ALL       0
#ifndef KEY_SIZE
# define KEY_SIZE        KEYSZ_128
#endif

//----- Choose whether table is in ROM or linked in -------------
// Default: Linked in
//
// Some Stellaris parts have the AES tables in ROM. If so, the application
// saves the space by not putting them in flash. The TABLE_IN_ROM
// define should be set if so. Note that there is only one table in ROM.
#ifndef TABLE_IN_ROM
# define TABLE_IN_ROM 0
#endif
#if TABLE_IN_ROM
# if TABLE_MODEL != ONE_TABLE
#  error When Table is in ROM, there is only one table
# endif
#endif


//-------------------------------------------------------
// Next section builds up data from selections and looks
// for problems
//-------------------------------------------------------

// determine size of expanded key in long words
#if KEY_SIZE==KEYSZ_256 || KEY_SIZE==KEYSZ_ALL
# define AESCONSZ       68      // 68 is needed for 256 bit
#else
# if KEY_SIZE==KEYSZ_192
#  define AESCONSZ       54     // 54 is needed for 192
# else
#  define AESCONSZ       44             // 44 is needed for 128
# endif
#endif

// define round passes based on key size
#define NUM_ROUNDS_128  10
#define NUM_ROUNDS_192  12
#define NUM_ROUNDS_256  14

#if KEY_FORM!=KEY_SET
// using pre-set key, so create declaration
# if KEY_SIZE==KEYSZ_ALL
#  error Pre-set key should only be used with single size
# endif
# if KEY_SIZE==KEYSZ_256
#  define FIXED_NUM_ROUNDS      NUM_ROUNDS_256
# endif
# if KEY_SIZE==KEYSZ_192
#  define FIXED_NUM_ROUNDS      NUM_ROUNDS_192
# endif
# if KEY_SIZE==KEYSZ_128
#  define FIXED_NUM_ROUNDS      NUM_ROUNDS_128
# endif

#endif                          // KEY_FORM!=KEY_SET
