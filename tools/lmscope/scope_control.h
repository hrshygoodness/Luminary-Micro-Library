// scope_control.h

#ifndef _SCOPE_CONTROL_H_
#define _SCOPE_CONTROL_H_

//****************************************************************************
//
// Window messages sent for asynchronous notifications. In all cases where
// LPARAM contains a pointer, the client is responsible for freeing that
// pointer using a call to LocalFree().
//
//****************************************************************************

// The device driver for the oscilloscope is not installed.
// WPARAM = 0, LPARAM = 0
#define WM_SCOPE_NO_DRIVER             (WM_USER + 0x100)

// The oscilloscope device is attached. Call ScopeControlConnect() to
// initiate communication.
// WPARAM = 0, LPARAM = 0
#define WM_SCOPE_DEVICE_AVAILABLE      (WM_USER + 0x101)

// The oscilloscope device is connected and communicating.
// WPARAM = 0, LPARAM = tScopeSettings *
#define WM_SCOPE_DEVICE_CONNECTED      (WM_USER + 0x102)

// The oscilloscope device has disconnected. Wait for a new
// WM_SCOPE_DEVICE_AVAILABLE before calling ScopeControlConnect() again
// to reinitiate communication.
// WPARAM = 0, LPARAM = 0
#define WM_SCOPE_DEVICE_DISCONNECTED   (WM_USER + 0x103)

// The oscilloscope device has sent capture data for the host. The pointer
// returned in LPARAM points to a buffer which starts with a tScopeDataStart
// structure and is followed by the waveform data. WPARAM contains the index
// of the first data sample within the LPARAM buffer.
// WPARAM = sample offset within buffer, LPARAM = tScopeDataStart *
#define WM_SCOPE_DATA                  (WM_USER + 0x104)

// The oscilloscope device has responded following a call to
// ScopeControlPing().
// WPARAM = ucEcho1, LPARAM = ulEcho2
#define WM_SCOPE_PING_RESPONSE         (WM_USER + 0x105)

// The oscilloscope trigger source and/or type has changed.
// WPARAM = SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2,
// LPARAM = SCOPE_TRIGGER_TYPE_LEVEL/RISING/FALLING/ALWAYS
#define WM_SCOPE_TRIGGER_TYPE_CHANGED  (WM_USER + 0x106)

// The oscilloscope trigger level has changed.
// WPARAM = 0, LPARAM = New trigger level in millivolts.
#define WM_SCOPE_TRIGGER_LEVEL_CHANGED (WM_USER + 0x107)

// The oscilloscope trigger position has changed.
// WPARAM = 0, LPARAM = New trigger position in pixels (-60, 60).
#define WM_SCOPE_TRIGGER_POS_CHANGED (WM_USER + 0x108)

// The oscilloscope has started automatic capture of waveform data.
// WPARAM = 0, LPARAM = 0
#define WM_SCOPE_STARTED               (WM_USER + 0x109)

// The oscilloscope has stopped automatic capture of waveform data.
// WPARAM = 0, LPARAM = 0
#define WM_SCOPE_STOPPED               (WM_USER + 0x10A)

// Channel 2 capture has been enabled or disabled.
// WPARAM = SCOPE_CHANNEL2_DISABLE or SCOPE_CHANNEL2_ENABLE, LPARAM = 0
#define WM_SCOPE_CHANNEL2              (WM_USER + 0x10B)

// The oscilloscope timebase had been changed.
// WPARAM = 0, LPARAM = new timebase expressed in microseconds per division.
#define WM_SCOPE_TIMEBASE_CHANGED      (WM_USER + 0x10C)

// The vertical position of one of the channel waveforms has changed.
// WPARAM = SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2, LPARAM = new offset in mV (signed).
#define WM_SCOPE_POS_CHANGED           (WM_USER + 0x10D)

// The vertical scale of one of the channel waveforms has changed.
// WPARAM = SCOPE_CHANNEL_1 or SCOPE_CHANNEL_2, LPARAM = new scale in mV/div.
#define WM_SCOPE_SCALE_CHANGED         (WM_USER + 0x10E)

//****************************************************************************
//
// Functions offered by the scope control module.
//
//****************************************************************************
extern bool ScopeControlInit(HWND hwndNotify);
extern bool ScopeControlTerm(void);
extern bool ScopeControlConnect(tScopeSettings *pSettings);
extern bool ScopeControlDisconnect(void);
extern bool ScopeControlStartStop(bool bStart);
extern bool ScopeControlPing(unsigned char ucEcho1, unsigned long ulEcho2);
extern bool ScopeControlCapture(void);
extern bool ScopeControlSetTimebase(unsigned long ulTimebaseuSdiv);
extern bool ScopeControlSetTriggerLevel(unsigned long ulLevelmV);
extern bool ScopeControlSetTrigger(unsigned char ucChannel,
                                   unsigned long ulType);
extern bool ScopeControlSetTriggerPos(long lPos);
extern bool ScopeControlSetPosition(unsigned char ucChannel,
                                    long lPosmV);
extern bool ScopeControlSetScale(unsigned char ucChannel,
                                 unsigned long ulScalemVdiv);
extern bool ScopeControlEnableChannel2(bool bEnable);
extern bool ScopeControlAutomaticDataTransmission(bool bEnable);
extern bool ScopeControlDataRequest(void);
extern bool ScopeControlReadData(tScopeDataStart *pStartInfo,
                                 unsigned long *pulNumElements,
                                 void **ppvData);
extern bool ScopeControlFind(unsigned char ucChannel);

#endif
