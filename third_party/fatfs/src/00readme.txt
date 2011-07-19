FatFs/Tiny-FatFs Module Source Files R0.04b   (C)ChaN, 2007


FILES

  ff.h       Common include file for FatFs and application module.
  ff.c       FatFs module.
  tff.h      Common include file for Tiny-FatFs and application module.
  tff.c      Tiny-FatFs module.
  diskio.h   Common include file for (Tiny-)FatFs and disk I/O module.
  integer.h  Alternative type definitions for integer variables.

  Low level disk I/O module is not included in this archive because the
  FatFs/Tiny-FatFs module is only a generic file system layer and not depend
  on any specific storage device. You have to provide a low level disk I/O
  module that written to control your storage device.



CONFIGURATION OPTIONS

  There are several configuration options for various envilonment and
  requirement. The configuration options are defined in header files, ff.h
  and tff.h.

  _MCU_ENDIAN

  This is the most impotant option that depends on the processor architecture.
  When the target device corresponds to either or both of following terms, the
  _MCU_ENDIAN must be set to 2 to force FatFs to access FAT structure in
  byte-by-byte.

  - Muti-byte integers (short, long) are stored in Big-Endian.
  - Address miss-aligned memory access results in an incorrect behavior.

  If not the case, this can be set to 1 for good code efficiency. The initial
  value is 0. (must be set to 1 or 2 properly)


  _FS_READONLY

  When application program does not require any write function, _FS_READONLY
  can be set to 1 to eliminate writing code to reduce the module size. This
  setting should be reflected to configurations of low level disk I/O module
  if available. The initial value is 0. (Read and Write)


  _FS_MINIMIZE

  When application program requires only file read/write function, _FS_MINIMIZE
  can be changed to eliminate some functions to reduce the module size. The
  initial value is 0. (full function)


  _DRIVES

  Number of drives to be used. This option is not available on Tiny-FatFs.
  The initial value is 2.

  _FAT32

  When _FAT32 is set to 1, the FAT32 support is added with an additional
  code size. This option is for only Tiny-FatFs. FatFs always supports all
  FAT sub-types. The initial value is 0. (no FAT32 support)


  _USE_FSINFO

  When _USE_FSINFO is set to 1, FSInfo is used for FAT32 volume.


  _USE_SJIS

  When _USE_SJIS is set to 1, Shift_JIS code set can be used as a file name,
  otherwire second byte of double-byte characters will be collapted.
  The initial value is 1.


  _USE_NTFLAG

  When _USE_NTFLAG is set to 1, upper/lower case of the file/dir name is
  preserved. Note that the files are always accessed in case insensitive.


  _USE_MKFS

  When _USE_MKFS is set to 1 and _FS_READONLY is set to 0, f_mkfs function
  is enabled. This is for only FatFs module and not supported on Tiny-FatFs.
  The initial value is 0. (f_mkfs is not available)


  Following table shows which function is removed by configuratin options.

                _FS_MINIMIZE   _FS_READONLY  _USE_MKFS 
                (1)  (2)  (3)       (1)         (0)    
   f_mount                                             
   f_open                                              
   f_close                                             
   f_read                                              
   f_write                           x                 
   f_sync                            x                 
   f_lseek                 x                           
   f_opendir          x    x                           
   f_readdir          x    x                           
   f_stat        x    x    x                           
   f_getfree     x    x    x         x                 
   f_unlink      x    x    x         x                 
   f_mkdir       x    x    x         x                 
   f_chmod       x    x    x         x                 
   f_rename      x    x    x         x                 
   f_mkfs        x    x    x         x          x      



AGREEMENTS

  The FatFs/Tiny-FatFs module is a free software and there is no warranty.
  The FatFs/Tiny-FatFs module is opened for education, reserch and development.
  You can use and/or modify it for personal, non-profit or profit use without
  any restriction under your responsibility.



REVISION HISTORY

  Feb 26, 2006  R0.00  Prototype

  Apr 29, 2006  R0.01  First release.

  Jun 01, 2006  R0.02  Added FAT12.
                       Removed unbuffered mode.
                       Fixed a problem on small (<32M) patition.

  Jun 10, 2006  R0.02a Added a configuration option _FS_MINIMUM.

  Sep 22, 2006  R0.03  Added f_rename().
                       Changed option _FS_MINIMUM to _FS_MINIMIZE.

  Dec 11, 2006  R0.03a Improved cluster scan algolithm to write files fast.
                       Fixed f_mkdir() creates incorrect directory on FAT32.

  Feb 04, 2007  R0.04  Supported multiple drive system. (FatFs)
                       Changed some APIs for multiple drive system.
                       Added f_mkfs(). (FatFs)
                       Added _USE_FAT32 option. (Tiny-FatFs)

  Apr 01, 2007  R0.04a Supported multiple partitions on a plysical drive. (FatFs)
                       Fixed an endian sensitive code in f_mkfs(). (FatFs)
                       Added a capability of extending the file size to f_lseek().
                       Added minimization level 3.
                       Fixed a problem that can collapse a sector when recreate an
                       existing file in any sub-directory at non FAT32 cfg. (Tiny-FatFs)

  May 05, 2007  R0.04b  Added _USE_NTFLAG option.
                        Added FSInfo support.
                        Fixed some problems corresponds to FAT32. (Tiny-FatFs)
                        Fixed DBCS name can result FR_INVALID_NAME.
                        Fixed short seek (<= csize) collapses the file object.
