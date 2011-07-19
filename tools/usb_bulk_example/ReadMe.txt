-----------------------------------
USB Bulk Example (usb_bulk_example)
-----------------------------------

This Windows command line application is written using C for Windows
and should be built using Microsoft Visual Studio 2005.  Project 
file usb_bulk_example.vcproj is provided to build the application.

USB communication with the EK-LM3S3748 board running the usb_dev_bulk
sample application is performed using the LMUSBDLL interface which is
a thin wrapper over the Microsoft WinUSB API.  The relevant WinUSB 
subsystem files and the LMUSBDLL.dll are installed when you first
connect the EK board via USB and install the Windows drivers.

The LMUSBDLL interface is provided purely to allow the sample applications
to be compiled and run in the absence of the Windows Device Driver Kit
(DDK).  The WinUSB API header and library files are not included in the 
Windows SDK shipped with Visual Studio 2005 so any code which uses this
interface requires access to the DDK to build.  LMUSBDLL contains all
the application code requiring WinUSB so applications may link to it
without the need for the Windows DDK.
