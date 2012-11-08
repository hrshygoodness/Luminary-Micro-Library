-------------------------------------
Luminary Micro Oscilloscope (LMScope)
-------------------------------------

This application is written using C++ and Microsoft Foundation Classes for
Windows and should be built using Microsoft Visual Studio 2005.  Project 
file lmscope.vcproj is provided to build the application.

USB communication with the EK-LM3S3748 or EK-LM3S3768 board running the
qs-scope sample application is performed using the LMUSBDLL interface 
which offers a thin layer over the Microsoft WinUSB API available on
WindowsXP and Vista.  The relevant subsystem files are installed when
you first connect the EK board via USB and install the Windows drivers.

The LMUSBDLL interface is provided purely to allow the sample applications
to be compiled and run in the absence of the Windows Device Driver Kit
(DDK).  The WinUSB API header and library files are not included in the 
Windows SDK shipped with Visual Studio 2005 so any code which uses this
interface requires access to the DDK to build.  LMUSBDLL contains all
the application code requiring WinUSB so applications may link to it
without the need for the Windows DDK.

To successfully build the LMScope application, your PC must have Microsoft
HTML Help Workshop installed in addition to Visual Studio 2005.

Microsoft HTML Help Workshop is used to create the application help file
(lmscope.chm) from HTML topic pages.  It can be freely downloaded from
Microsoft at

http://msdn2.microsoft.com/en-us/library/ms669985.aspx
