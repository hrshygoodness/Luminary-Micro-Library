//
// Public header for the USB interface used by the usb_bulk_example application.
//

#ifndef __BULK_USB_H__
#define __BULK_USB_H__

extern BOOL InitializeDevice(void);
extern BOOL TerminateDevice(void);
extern BOOL WriteUSBPacket(unsigned char *pcBuffer, unsigned long ulSize,
                           unsigned long *pulWritten);
extern BOOL ReadUSBPacket(unsigned char *pcBuffer, unsigned long ulSize,
                          unsigned long *pulRead);

#endif