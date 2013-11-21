//*****************************************************************************
//
// simple_fs.c - Functions for simple FAT file system support
//
// Copyright (c) 2009-2013 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 10636 of the EK-LM3S9D90 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "simple_fs.h"

//*****************************************************************************
//
//! \addtogroup simple_fs_api
//! @{
//!
//! This file system API should be used as follows:
//! - Initialize it by calling SimpleFsInit().  You must supply a pointer to a
//! 512 byte buffer that will be used for storing device sector data.
//! - "Open" a file by calling SimpleFsOpen() and passing the 8.3-style filename
//! as an 11-character string.
//! - Read successive sectors from the file by using the convenience macro
//! SimpleFsReadFileSector().
//!
//! This API does not use any file handles so there is no way to open more than
//! one file at a time.  There is also no random access into the file, each
//! sector must be read in sequence.
//!
//! The client of this API supplies a 512-byte buffer for storage of data read
//! from the device.  But this file also maintains an additional, internal
//! 512-byte buffer used for caching FAT sectors.  This minimizes the amount
//! of device reads required to fetch cluster chain entries from the FAT.
//!
//! The application code (the client) must also provide a function used for
//! reading sectors from the storage device, whatever it may be.  This allows
//! the code in this file to be independent of the type of device used for
//! storing the file system.  The name of the function is
//! SimpleFsReadMediaSector().
//
//*****************************************************************************

//*****************************************************************************
//
// Setup a macro for handling packed data structures.
//
//*****************************************************************************
#if defined(ccs) ||                                                           \
    defined(codered) ||                                                       \
    defined(gcc) ||                                                           \
    defined(rvmdk) ||                                                         \
    defined(__ARMCC_VERSION) ||                                               \
    defined(sourcerygxx)
#define PACKED                  __attribute__((packed))
#elif defined(ewarm)
#define PACKED
#else
#error "Unrecognized COMPILER!"
#endif

//*****************************************************************************
//
// Instruct the IAR compiler to pack the following structures.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack(1)
#endif

//*****************************************************************************
//
// Structures for mapping FAT file system
//
//*****************************************************************************

//*****************************************************************************
//
// The FAT16 boot sector extension
//
//*****************************************************************************
typedef struct
{
    unsigned char ucDriveNumber;
    unsigned char ucReserved;
    unsigned char ucExtSig;
    unsigned long ulSerial;
    char cVolumeLabel[11];
    char cFsType[8];
    unsigned char ucBootCode[448];
    unsigned short usSig;
}
PACKED tBootExt16;

//*****************************************************************************
//
// The FAT32 boot sector extension
//
//*****************************************************************************
typedef struct
{
    unsigned long ulSectorsPerFAT;
    unsigned short usFlags;
    unsigned short usVersion;
    unsigned long ulRootCluster;
    unsigned short usInfoSector;
    unsigned short usBootCopy;
    unsigned char ucReserved[12];
    unsigned char ucDriveNumber;
    unsigned char ucReserved1;
    unsigned char ucExtSig;
    unsigned long ulSerial;
    char cVolumeLabel[11];
    char cFsType[8];
    unsigned char ucBootCode[420];
    unsigned short usSig;
}
PACKED tBootExt32;

//*****************************************************************************
//
// The FAT16/32 boot sector main section
//
//*****************************************************************************
typedef struct
{
    unsigned char ucJump[3];
    char cOEMName[8];
    unsigned short usBytesPerSector;
    unsigned char ucSectorsPerCluster;
    unsigned short usReservedSectors;
    unsigned char ucNumFATs;
    unsigned short usNumRootEntries;
    unsigned short usTotalSectorsSmall;
    unsigned char ucMediaDescriptor;
    unsigned short usSectorsPerFAT;
    unsigned short usSectorsPerTrack;
    unsigned short usNumberHeads;
    unsigned long ulHiddenSectors;
    unsigned long ulTotalSectorsBig;
    union
    {
        tBootExt16 sExt16;
        tBootExt32 sExt32;
    }
    PACKED ext;
}
PACKED tBootSector;

//*****************************************************************************
//
// The partition table
//
//*****************************************************************************
typedef struct
{
    unsigned char ucStatus;
    unsigned char ucCHSFirst[3];
    unsigned char ucType;
    unsigned char ucCHSLast[3];
    unsigned long ulFirstSector;
    unsigned long ulNumBlocks;
}
PACKED tPartitionTable;

//*****************************************************************************
//
// The master boot record (MBR)
//
//*****************************************************************************
typedef struct
{
    unsigned char ucCodeArea[440];
    unsigned char ucDiskSignature[4];
    unsigned char ucNulls[2];
    tPartitionTable sPartTable[4];
    unsigned short usSig;
}
PACKED tMasterBootRecord;

//*****************************************************************************
//
// The structure for a single directory entry
//
//*****************************************************************************
typedef struct
{
    char cFileName[11];
    unsigned char ucAttr;
    unsigned char ucReserved;
    unsigned char ucCreateTime[5];
    unsigned char ucLastDate[2];
    unsigned short usClusterHi;
    unsigned char ucLastModified[4];
    unsigned short usCluster;
    unsigned long ulFileSize;
}
PACKED tDirEntry;

//*****************************************************************************
//
// Tell the IAR compiler that the remaining structures do not need to be
// packed.
//
//*****************************************************************************
#ifdef ewarm
#pragma pack()
#endif

//*****************************************************************************
//
// This structure holds information about the layout of the file system
//
//*****************************************************************************
typedef struct
{
    unsigned long ulFirstSector;
    unsigned long ulNumBlocks;
    unsigned short usSectorsPerCluster;
    unsigned short usMaxRootEntries;
    unsigned long ulSectorsPerFAT;
    unsigned long ulFirstFATSector;
    unsigned long ulLastFATSector;
    unsigned long ulFirstDataSector;
    unsigned long ulType;
    unsigned long ulStartRootDir;
}
tPartitionInfo;

static tPartitionInfo sPartInfo;

//*****************************************************************************
//
// A pointer to the client provided sector buffer.
//
//*****************************************************************************
static unsigned char *g_pucSectorBuf;

//*****************************************************************************
//
//! Initializes the simple file system
//!
//! \param pucSectorBuf is a pointer to a caller supplied 512-byte buffer
//! that will be used for holding sectors that are loaded from the media
//! storage device.
//!
//! Reads the MBR, partition table, and boot record to find the logical
//! structure of the file system.  This function stores the file system
//! structural data internally so that the remaining functions of the API
//! can read the file system.
//!
//! To read data from the storage device, the function SimpleFsReadMediaSector()
//! will be called.  This function is not implemented here but must be
//! implemented by the user of this simple file system.
//!
//! This file system support is extremely simple-minded.  It will only
//! find the first partition of a FAT16 or FAT32 formatted mass storage
//! device.  Only very minimal error checking is performed in order to save
//! code space.
//!
//! \return Zero if successful, non-zero if there was an error.
//
//*****************************************************************************
unsigned long
SimpleFsInit(unsigned char *pucSectorBuf)
{
    tMasterBootRecord *pMBR;
    tPartitionTable *pPart;
    tBootSector *pBoot;

    //
    // Save the sector buffer pointer.  The input parameter is assumed
    // to be good.
    //
    g_pucSectorBuf = pucSectorBuf;

    //
    // Get the MBR
    //
    if(SimpleFsReadMediaSector(0, pucSectorBuf))
    {
        return(1);
    }

    //
    // Verify MBR signature - bare minimum validation of MBR.
    //
    pMBR = (tMasterBootRecord *)pucSectorBuf;
    if(pMBR->usSig != 0xAA55)
    {
        return(1);
    }

    //
    // See if this is a MBR or a boot sector.
    //
    pBoot = (tBootSector *)pucSectorBuf;
    if((strncmp(pBoot->ext.sExt16.cFsType, "FAT", 3) != 0) &&
       (strncmp(pBoot->ext.sExt32.cFsType, "FAT32", 5) != 0))
    {
        //
        // Get the first partition table
        //
        pPart = &(pMBR->sPartTable[0]);

        //
        // Could optionally check partition type here ...
        //

        //
        // Get the partition location and size
        //
        sPartInfo.ulFirstSector = pPart->ulFirstSector;
        sPartInfo.ulNumBlocks = pPart->ulNumBlocks;

        //
        // Read the boot sector from the partition
        //
        if(SimpleFsReadMediaSector(sPartInfo.ulFirstSector, pucSectorBuf))
        {
            return(1);
        }
    }
    else
    {
        //
        // Extract the number of sectors from the boot sector.
        //
        sPartInfo.ulFirstSector = 0;
        if(pBoot->usTotalSectorsSmall == 0)
        {
            sPartInfo.ulNumBlocks = pBoot->ulTotalSectorsBig;
        }
        else
        {
            sPartInfo.ulNumBlocks = pBoot->usTotalSectorsSmall;
        }
    }

    //
    // Get pointer to the boot sector
    //
    if(pBoot->ext.sExt16.usSig != 0xAA55)
    {
        return(1);
    }

    //
    // Verify the sector size is 512.  We can't deal with anything else
    //
    if(pBoot->usBytesPerSector != 512)
    {
        return(1);
    }

    //
    // Extract some info from the boot record
    //
    sPartInfo.usSectorsPerCluster = pBoot->ucSectorsPerCluster;
    sPartInfo.usMaxRootEntries = pBoot->usNumRootEntries;

    //
    // Decide if we are dealing with FAT16 or FAT32.
    // If number of root entries is 0, that suggests FAT32
    //
    if(sPartInfo.usMaxRootEntries == 0)
    {
        //
        // Confirm FAT 32 signature in the expected place
        //
        if(!strncmp(pBoot->ext.sExt32.cFsType, "FAT32   ", 8))
        {
            sPartInfo.ulType = 32;
        }
        else
        {
            return(1);
        }
    }
    //
    // Root entries is non-zero, suggests FAT16
    //
    else
    {
        //
        // Confirm FAT16 signature
        //
        if(!strncmp(pBoot->ext.sExt16.cFsType, "FAT16   ", 8))
        {
            sPartInfo.ulType = 16;
        }
        else
        {
            return(1);
        }
    }

    //
    // Find the beginning of the FAT, in absolute sectors
    //
    sPartInfo.ulFirstFATSector = sPartInfo.ulFirstSector +
                                 pBoot->usReservedSectors;

    //
    // Find the end of the FAT in absolute sectors.  FAT16 and 32
    // are handled differently.
    //
    sPartInfo.ulSectorsPerFAT = (sPartInfo.ulType == 16) ?
                                pBoot->usSectorsPerFAT :
                                pBoot->ext.sExt32.ulSectorsPerFAT;
    sPartInfo.ulLastFATSector = sPartInfo.ulFirstFATSector +
                                sPartInfo.ulSectorsPerFAT - 1;

    //
    // Find the start of the root directory and the data area.
    // For FAT16, the root will be stored as an absolute sector number
    // For FAT32, the root will be stored as the starting cluster of the root
    // The data area start is the absolute first sector of the data area.
    //
    if(sPartInfo.ulType == 16)
    {
        sPartInfo.ulStartRootDir = sPartInfo.ulFirstFATSector +
                                   (sPartInfo.ulSectorsPerFAT *
                                    pBoot->ucNumFATs);
        sPartInfo.ulFirstDataSector = sPartInfo.ulStartRootDir +
                                      (sPartInfo.usMaxRootEntries / 16);
    }
    else
    {
        sPartInfo.ulStartRootDir = pBoot->ext.sExt32.ulRootCluster;
        sPartInfo.ulFirstDataSector = sPartInfo.ulFirstFATSector +
                                      (sPartInfo.ulSectorsPerFAT * pBoot->ucNumFATs);
    }

    //
    // At this point the file system has been initialized, so return
    // success to the caller.
    //
    return(0);
}

//*****************************************************************************
//
//! Find the next cluster in a FAT chain
//!
//! \param ulThisCluster is the current cluster in the chain
//!
//! Reads the File Allocation Table (FAT) of the file system to find the
//! next cluster in a chain of clusters.  The current cluster is passed in
//! and the next cluster in the chain will be returned.
//!
//! This function reads sectors from the storage device as needed in order
//! to parse the FAT tables.  Error handling is minimal since there is not
//! much that can be done if an error is encountered.  If any error is
//! encountered, or if this is the last cluster in the chain, then 0 is
//! returned.  This signals the caller to stop traversing the chain (either
//! due to error or end of chain).
//!
//! The function maintains a cache of a single sector from the FAT.  It only
//! reads in a new FAT sector if the requested cluster is not in the
//! currently cached sector.
//!
//! \return Next cluster number if successful, 0 if this is the last cluster
//! or any error is found.
//
//*****************************************************************************
static unsigned long
SimpleFsGetNextCluster(unsigned long ulThisCluster)
{
    static unsigned char ucFATCache[512];
    static unsigned long ulCachedFATSector = (unsigned long)-1;
    unsigned long ulClustersPerFATSector;
    unsigned long ulClusterIdx;
    unsigned long ulFATSector;
    unsigned long ulNextCluster;
    unsigned long ulMaxCluster;

    //
    // Compute the maximum possible reasonable cluster number
    //
    ulMaxCluster = sPartInfo.ulNumBlocks / sPartInfo.usSectorsPerCluster;

    //
    // Make sure cluster input number is reasonable.  If not then return
    // 0 indicating error.
    //
    if((ulThisCluster < 2) || (ulThisCluster > ulMaxCluster))
    {
        return(0);
    }

    //
    // Compute the index of the requested cluster within the sector.
    // Also compute the sector number within the FAT that contains the
    // entry for the requested cluster.
    //
    ulClustersPerFATSector = (sPartInfo.ulType == 16) ? 256 : 128;
    ulClusterIdx = ulThisCluster % ulClustersPerFATSector;
    ulFATSector = ulThisCluster / ulClustersPerFATSector;

    //
    // Check to see if the FAT sector we need is already cached
    //
    if(ulFATSector != ulCachedFATSector)
    {
        //
        // FAT sector we need is not cached, so read it in
        //
        if(SimpleFsReadMediaSector(sPartInfo.ulFirstFATSector + ulFATSector,
                                 ucFATCache) != 0)
        {
            //
            // There was an error so mark cache as unavailable and return
            // an error.
            //
            ulCachedFATSector = (unsigned long)-1;
            return(0);
        }

        //
        // Remember which FAT sector was just loaded into the cache.
        //
        ulCachedFATSector = ulFATSector;
    }

    //
    // Now look up the next cluster value from the cached sector, using this
    // requested cluster as an index. It needs to be indexed as 16 or 32
    // bit values depending on whether it is FAT16 or 32
    // If the cluster value means last cluster, then return 0
    //
    if(sPartInfo.ulType == 16)
    {
        ulNextCluster = ((unsigned short *)ucFATCache)[ulClusterIdx];
        if(ulNextCluster >= 0xFFF8)
        {
            return(0);
        }
    }
    else
    {
        ulNextCluster = ((unsigned long *)ucFATCache)[ulClusterIdx];
        if(ulNextCluster >= 0x0FFFFFF8)
        {
            return(0);
        }
    }

    //
    // Check new cluster value to make sure it is reasonable.  If not then
    // return 0 to indicate an error.
    //
    if((ulNextCluster >= 2) && (ulNextCluster <= ulMaxCluster))
    {
        return(ulNextCluster);
    }
    else
    {
        return(0);
    }
}

//*****************************************************************************
//
//! Read a single sector from a file into the sector buffer
//!
//! \param ulStartCluster is the first cluster of the file, used to
//! initialize the file read.  Use 0 for successive sectors.
//!
//! Reads sectors in sequence from a file and stores the data in the sector
//! buffer that was passed in the initial call to SimpleFsInit().  The function
//! is initialized with the file to read by passing the starting cluster of
//! the file.  The function will initialize some static data and return.  It
//! does not read any file data when passed a starting cluster (and
//! returns 0 - this is normal).
//!
//! Once the function has been initialized with the file's starting cluster,
//! then successive calls should be made, passing a value of 0 for the
//! cluster number.  This tells the function to read the next sector from the
//! file and store it in the sector buffer.  The function remembers the last
//! sector that was read, and each time it is called with a cluster value of
//! 0, it will read the next sector.  The function will traverse the FAT
//! chain as needed to read all the sectors.  When a sector has been
//! successfully read from a file, the function will return non-zero.  When
//! there are no more sectors to read, or any error is encountered, the
//! function will return 0.
//!
//! Note that the function always reads a whole sector, even if the end of
//! a file does not fill the last sector.  It is the responsibility of the
//! caller to track the file size and to deal with a partially full last
//! sector.
//!
//! \return Non-zero if a sector was read into the sector buffer, or
//! 0 if there are no more sectors or if any error occurred.
//
//*****************************************************************************
unsigned long
SimpleFsGetNextFileSector(unsigned long ulStartCluster)
{
    static unsigned long ulWorkingCluster = 0;
    static unsigned long ulWorkingSector;
    unsigned long ulReadSector;

    //
    // If user specified starting cluster, then init the working cluster
    // and sector values
    //
    if(ulStartCluster)
    {
        ulWorkingCluster = ulStartCluster;
        ulWorkingSector = 0;
        return(0);
    }

    //
    // Otherwise, make sure there is a valid working cluster already
    //
    else if(ulWorkingCluster == 0)
    {
        return(0);
    }

    //
    // If the current working sector is the same as sectors per cluster,
    // then that means that the next cluster needs to be loaded.
    //
    if(ulWorkingSector == sPartInfo.usSectorsPerCluster)
    {
        //
        // Get the next cluster in the chain for this file.
        //
        ulWorkingCluster = SimpleFsGetNextCluster(ulWorkingCluster);

        //
        // If the next cluster is valid, then reset the working sector
        //
        if(ulWorkingCluster)
        {
            ulWorkingSector = 0;
        }

        //
        // Next cluster is not valid, or this was the end of the chain.
        // Clear the working cluster and return an indication that no new
        // sector data was loaded.
        //
        else
        {
            ulWorkingCluster = 0;
            return(0);
        }
    }

    //
    // Calculate the sector to read from.  It is the sector of the start
    // of the working cluster, plus the working sector (the sector within
    // the cluster), plus the offset to the start of the data area.
    // Note that the cluster needs to be reduced by 2 in order to index
    // properly into the data area.  That is a feature of FAT file system.
    //
    ulReadSector = (ulWorkingCluster - 2) * sPartInfo.usSectorsPerCluster;
    ulReadSector += ulWorkingSector;
    ulReadSector += sPartInfo.ulFirstDataSector;

    //
    // Attempt to read the next sector from the cluster.  If not successful,
    // then clear the working cluster and return a non-success indication.
    //
    if(SimpleFsReadMediaSector(ulReadSector, g_pucSectorBuf) != 0)
    {
        ulWorkingCluster = 0;
        return(0);
    }
    else
    {
        //
        // Read was successful.  Increment to the next sector of the cluster
        // and return a success indication.
        //
        ulWorkingSector++;
        return(1);
    }
}

//*****************************************************************************
//
//! Find a file in the root directory of the file system and open it for
//! reading.
//!
//! \param pcName83 is an 11-character string that represents the 8.3 file
//! name of the file to open.
//!
//! This function traverses the root directory of the file system to find
//! the file name specified by the caller.  Note that the file name must be
//! an 8.3 file name that is 11 characters long.  The first 8 characters are
//! the base name and the last 3 characters are the extension.  If there are
//! fewer characters in the base name or extension, the name should be padded
//! with spaces.  For example "myfile.bn" has fewer than 11 characters, and
//! should be passed with padding like this: "myfile  bn ".  Note the extra
//! spaces, and that the dot ('.') is not part of the string that is passed
//! to this function.
//!
//! If the file is found, then it initializes the file for reading, and returns
//! the file length.  The file can be read by making successive calls to
//! SimpleFsReadFileSector().
//!
//! The function only searches the root directory and ignores any
//! subdirectories.  It also ignores any long file name entries, looking only
//! at the 8.3 file name for a match.
//!
//! \return The size of the file if it is found, or 0 if the file could not
//! be found.
//
//*****************************************************************************
unsigned long
SimpleFsOpen(char *pcName83)
{
    tDirEntry *pDirEntry;
    unsigned long ulDirSector;
    unsigned long ulFirstCluster;

    //
    // Find starting root dir sector, only used for FAT16
    // If FAT32 then this is the first cluster of root dir
    //
    ulDirSector = sPartInfo.ulStartRootDir;

    //
    // For FAT32, root dir is like a file, so init a file read of the root dir
    //
    if(sPartInfo.ulType == 32)
    {
        SimpleFsGetNextFileSector(ulDirSector);
    }

    //
    // Search the root directory entry for the firmware file
    //
    while(1)
    {
        //
        // Read in a directory block.
        //
        if(sPartInfo.ulType == 16)
        {
            //
            // For FAT16, read in a sector of the root directory
            //
            if(SimpleFsReadMediaSector(ulDirSector, g_pucSectorBuf))
            {
                return(0);
            }
        }
        else
        {
            //
            // For FAT32, the root directory is treated like a file.
            // The root directory sector will be loaded into the sector buf
            //
            if(SimpleFsGetNextFileSector(0) == 0)
            {
                return(0);
            }
        }

        //
        // Initialize the directory entry pointer to the first entry of
        // this sector.
        //
        pDirEntry = (tDirEntry *)g_pucSectorBuf;

        //
        // Iterate through all the directory entries in this sector
        //
        while((unsigned char *)pDirEntry < &g_pucSectorBuf[512])
        {
            //
            // If the 8.3 filename of this entry matches the firmware
            // file name, then we have a match, so return a pointer to
            // this entry.
            //
            if(!strncmp(pDirEntry->cFileName, pcName83, 11))
            {
                //
                // Compute the starting cluster of the file
                //
                ulFirstCluster = pDirEntry->usCluster;
                if(sPartInfo.ulType == 32)
                {
                    //
                    // For FAT32, add in the upper word of the
                    // starting cluster number
                    //
                    ulFirstCluster += pDirEntry->usClusterHi << 16;
                }

                //
                // Initialize the start of the file
                //
                SimpleFsGetNextFileSector(ulFirstCluster);
                return(pDirEntry->ulFileSize);
            }

            //
            // Advance to the next entry in this sector.
            //
            pDirEntry++;
        }

        //
        // Need to get the next sector in the directory.  Handled
        // differently depending on if this is FAT16 or 32
        //
        if(sPartInfo.ulType == 16)
        {
            //
            // FAT16: advance sectors as long as there are more possible
            // entries.
            //
            sPartInfo.usMaxRootEntries -= 512 / 32;
            if(sPartInfo.usMaxRootEntries)
            {
                ulDirSector++;
            }
            else
            {
                //
                // Ran out of directory entries and didn't find the file,
                // so return a null.
                //
                return(0);
            }
        }
        else
        {
            //
            // FAT32: there is nothing to compute here.  The next root
            // dir sector will be fetched at the top of the loop
            //
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
