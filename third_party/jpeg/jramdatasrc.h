/*
 * jramdatasrc.h
 *
 * RAM-based data source module for the Independent JPEG Group's JPEG
 * decoder implementation.  This file is based on the jdatasrc.c file
 * provided with the reference decoder.
 */
#ifndef _JRAMDATASRC_H_
#define _JRAMDATASRC_H_

GLOBAL(boolean)
jpeg_ram_src (j_decompress_ptr cinfo, JOCTET *pucData, unsigned long ulSize);

#endif /* _JRAMDATASRC_H_ */
