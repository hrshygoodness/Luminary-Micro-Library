--------------------------------
Stellaris USB DLL (lmusbdll.dll)
--------------------------------

The project found in this directory builds a DLL used by both the lmscope
and usb_bulk_example applications to read and write USB data.  It acts as
a thin wrapper layer above the Microsoft WinUSB API and is intended purely
to allow the applications to be modified, rebuilt and run on development
systems which do not have the Windows Device Driver Kit installed.

To successfully rebuild the LMUSBDLL itself, your PC must have the Windows
Device Driver Kit (DDK) installed in directory C:\WinDDK\6000.  If you have
used a different path, you will need to modify the project settings to
change the include path appropriately.

The Windows DDK is available for free download from Microsoft at

http://www.microsoft.com/whdc/devtools/ddk/default.mspx
