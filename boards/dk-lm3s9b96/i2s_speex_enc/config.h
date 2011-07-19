/* config.h. These are the configuration options that are used with Stellaris
 * microcontrollers.  */

#ifndef __CONFIG_H__
#define __CONFIG_H__

/* Disable all parts of the API that are using floats */
#define DISABLE_FLOAT_API

/* Disable VBR and VAD from the codec */
#define DISABLE_VBR
#define DISABLE_WIDEBAND

/* Compile as fixed-point */
#define FIXED_POINT

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Version extra */
#define SPEEX_EXTRA_VERSION ""

/* Version major */
#define SPEEX_MAJOR_VERSION 1

/* Version micro */
#define SPEEX_MICRO_VERSION 16

/* Version minor */
#define SPEEX_MINOR_VERSION 1

/* Complete version string */
#define SPEEX_VERSION "1.2rc1"

/* Use KISS Fast Fourier Transform */
#define USE_KISS_FFT

/* Override the malloc and calloc calls with bget versions. */
#define calloc bgetz
#define malloc bget
#define realloc bgetr
#define free brel

/* Override the fatal error call. */
#define OVERRIDE_SPEEX_FATAL
void _speex_fatal(const char *str, const char *file, int line);

/* Override the putc call so that the library will not be included. */
#define OVERRIDE_SPEEX_PUTC
void _speex_putc(int ch, void *file);

/* Disable all warnings and other notifications. */
#define DISABLE_WARNINGS
#define DISABLE_NOTIFICATIONS

/* Allocate 4K to the scratch area for the encoder. */
#define NB_ENC_STACK (4*1024)

/* Only the Keil RVMDK tools need this definition.  */
#if defined(rvmdk)
#define inline __inline
#endif

/* Symbol visibility prefix */
#define EXPORT

/* Enable some the Stellaris optimizations based on only supporting narrow
 * band encode/decode. */
#define ENABLE_MISC_OPT
#define FIXED_LPC_SIZE          10
#define FIXED_SUBFRAME_SIZE     40
#define FIXED_FRAME_SIZE        160
#define FIXED_WINDOW_SIZE       (FIXED_FRAME_SIZE + FIXED_SUBFRAME_SIZE)

/* Define to equivalent of C99 restrict keyword, or to nothing if this is not
   supported. Do not define if restrict is supported directly. */
#define restrict __restrict
#endif
