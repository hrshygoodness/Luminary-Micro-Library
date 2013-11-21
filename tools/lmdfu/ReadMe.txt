========================================================================
          Stellaris Device Firmware Upgrade Interface Library
========================================================================

This project contains a DLL offering access to the USB Device Firmware
Upgrade function available on Stellaris USB-enabled microcontrollers.
The API here may be used to download new application binaries to devices
running the Stellaris USB boot loader and also to perform various
housekeeping operations (flash erase, image verification, etc.)

lmdfu.h
    This is the main header file for the DLL.  It contains prototypes for
    all the exported functions.

lmdfu.cpp
    This is the main DLL source file.  It contains the implementation for
    each function.

lmdfu.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++.

res\lmdfu.rc2
    This file contains resources that are not edited by Microsoft 
    Visual C++.  You should place all resources not editable by
    the resource editor in this file.

lmdfu.def
    This file contains information about the DLL that must be
    provided to run with Microsoft Windows.  It defines parameters
    such as the name and description of the DLL.  It also exports
    functions from the DLL.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named lmdfu.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
