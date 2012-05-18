//*****************************************************************************
//
// acquire.c - Data acquisition module for data logger application.
//
// Copyright (c) 2011-2012 Texas Instruments Incorporated.  All rights reserved.
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
// This is part of revision 8555 of the EK-LM4F232 Firmware Package.
//
//*****************************************************************************

#include <string.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "grlib/grlib.h"
#include "grlib/widget.h"
#include "grlib/canvas.h"
#include "utils/ustdlib.h"
#include "drivers/slidemenuwidget.h"
#include "stripchartwidget.h"
#include "stripchartmanager.h"
#include "clocksetwidget.h"
#include "qs-logger.h"
#include "usbstick.h"
#include "usbserial.h"
#include "flashstore.h"
#include "menus.h"
#include "acquire.h"

//*****************************************************************************
//
// This is the data acquisition module.  It performs acquisition of data from
// selected channels, starting and stopping data logging, storing acquired
// data, and running the strip chart display.
//
//*****************************************************************************

//*****************************************************************************
//
// The following defines which ADC channel control should be used for each
// kind of data item.  Basically it maps how the ADC channels are connected
// on the board.  This is a hardware configuration.
//
//*****************************************************************************
#define CHAN_USER0      ADC_CTL_CH0
#define CHAN_USER1      ADC_CTL_CH1
#define CHAN_USER2      ADC_CTL_CH2
#define CHAN_USER3      ADC_CTL_CH3
#define CHAN_ACCELX     ADC_CTL_CH8
#define CHAN_ACCELY     ADC_CTL_CH9
#define CHAN_ACCELZ     ADC_CTL_CH21
#define CHAN_EXTTEMP    ADC_CTL_CH20
#define CHAN_CURRENT    ADC_CTL_CH23
#define CHAN_INTTEMP    ADC_CTL_TS

//*****************************************************************************
//
// The following maps the order that items are acquired and stored by the
// ADC sequencers.  Note that 16 samples are specified, using 2 of the
// 8 sample sequencers.  The current is sampled multiple times deliberately
// because that value tends to bounce around.  It is sampled multiple
// times and will be averaged.
//
//*****************************************************************************
unsigned long g_ulADCSeq[] =
{
    CHAN_USER0, CHAN_USER1, CHAN_USER2, CHAN_USER3,
    CHAN_ACCELX, CHAN_ACCELY, CHAN_ACCELZ, CHAN_EXTTEMP,
    CHAN_INTTEMP, CHAN_CURRENT, CHAN_CURRENT, CHAN_CURRENT,
    CHAN_CURRENT, CHAN_CURRENT, CHAN_CURRENT, CHAN_CURRENT,
};
#define NUM_ADC_CHANNELS (sizeof(g_ulADCSeq) / sizeof(unsigned long))
#define NUM_CURRENT_SAMPLES 7

//*****************************************************************************
//
// A buffer to hold one set of ADC data that is acquired per sample time.
//
//*****************************************************************************
static unsigned long g_ulADCData[NUM_ADC_CHANNELS];

//*****************************************************************************
//
// The following variables hold the current time stamp, the next match time
// for sampling, and the period of time between samples.  All are stored in
// a 32.15 second.subsecond format.
//
//*****************************************************************************
static volatile unsigned long g_ulTimeStamp[2];
static volatile unsigned long g_ulNextMatch[2];
static unsigned long g_ulMatchPeriod[2];

//*****************************************************************************
//
// The number of data items that are selected for acquisition.
//
//*****************************************************************************
static unsigned long g_ulNumItems;

//*****************************************************************************
//
// A counter for the ADC interrupt handler.  It is used to track when new
// ADC data is acquired.
//
//*****************************************************************************
static volatile unsigned long g_ulADCCount;

//*****************************************************************************
//
// A counter for the RTC interrupt handler.
//
//*****************************************************************************
static volatile unsigned long g_ulRTCInts;

//*****************************************************************************
//
// A flag to indicate that a keep alive packet is needed (when logging to
// host PC).
//
//*****************************************************************************
static volatile tBoolean g_bNeedKeepAlive = 0;

//*****************************************************************************
//
// Storage for a single record of acquired data.  This needs to be large
// enough to hold the time stamp and item mask (defined in the structure
// above) and as many possible data items that can be collected.  Force the
// buffer to be a multiple of 32-bits.
//
//*****************************************************************************
#define RECORD_SIZE (sizeof(tLogRecord) + ((NUM_LOG_ITEMS - 1) * 2))
static union
{
    unsigned long g_ulRecordBuf[(RECORD_SIZE + 2) / sizeof(unsigned long)];
    tLogRecord sRecord;
} g_sRecordBuf;

//*****************************************************************************
//
// Holds a pointer to the current configuration state, that is determined by
// the user's menu selections.
//
//*****************************************************************************
static tConfigState *g_pConfigState;

//*****************************************************************************
//
// This function is called when in VIEW mode.  The acquired data is written
// as text strings which will appear on the eval board display.
//
//*****************************************************************************
static void
UpdateViewerData(const tLogRecord *pRecord)
{
    static char cViewerBuf[24];
    unsigned long ulIdx;
    tTime sTime;
    unsigned long ulRTC;

    //
    // Loop through the analog channels and update the text display strings.
    //
    for(ulIdx = LOG_ITEM_USER0; ulIdx <= LOG_ITEM_USER3; ulIdx++)
    {
        usnprintf(cViewerBuf, sizeof(cViewerBuf), " CH%u: %u.%03u V ",
                  ulIdx - LOG_ITEM_USER0,
                  pRecord->sItems[ulIdx] / 1000,
                  pRecord->sItems[ulIdx] % 1000);
        MenuUpdateText(ulIdx, cViewerBuf);
    }

    //
    // Loop through the accel channels and update the text display strings.
    //
    for(ulIdx = LOG_ITEM_ACCELX; ulIdx <= LOG_ITEM_ACCELZ; ulIdx++)
    {
        short sAccel = pRecord->sItems[ulIdx];
        sAccel *= (sAccel < 0) ? -1 : 1;
        usnprintf(cViewerBuf, sizeof(cViewerBuf), " %c: %c%d.%02u g ",
                  (ulIdx - LOG_ITEM_ACCELX) + 'X',
                  pRecord->sItems[ulIdx] < 0 ? '-' : '+',
                  sAccel / 100, sAccel % 100);
        MenuUpdateText(ulIdx, cViewerBuf);
    }

    //
    // Update the display string for internal temperature.
    //
    usnprintf(cViewerBuf, sizeof(cViewerBuf), " INT: %d.%01u C ",
              pRecord->sItems[LOG_ITEM_INTTEMP] / 10,
              pRecord->sItems[LOG_ITEM_INTTEMP] % 10);
    MenuUpdateText(LOG_ITEM_INTTEMP, cViewerBuf);

    //
    // Update the display string for external temperature.
    //
    usnprintf(cViewerBuf, sizeof(cViewerBuf), " EXT: %d.%01u C ",
              pRecord->sItems[LOG_ITEM_EXTTEMP] / 10,
              pRecord->sItems[LOG_ITEM_EXTTEMP] % 10);
    MenuUpdateText(LOG_ITEM_EXTTEMP, cViewerBuf);

    //
    // Update the display string for processor current.
    //
    usnprintf(cViewerBuf, sizeof(cViewerBuf), " %u.%01u mA ",
              pRecord->sItems[LOG_ITEM_CURRENT] / 10,
              pRecord->sItems[LOG_ITEM_CURRENT] % 10);
    MenuUpdateText(LOG_ITEM_CURRENT, cViewerBuf);

    //
    // Update the display strings for time and data.
    //
    ulRTC = HibernateRTCGet();
    ulocaltime(ulRTC, &sTime);
    usnprintf(cViewerBuf, sizeof(cViewerBuf), "%4u/%02u/%02u",
              sTime.usYear, sTime.ucMon + 1, sTime.ucMday);
    MenuUpdateText(TEXT_ITEM_DATE, cViewerBuf);
    usnprintf(cViewerBuf, sizeof(cViewerBuf), "%02u:%02u:%02u",
              sTime.ucHour, sTime.ucMin, sTime.ucSec);
    MenuUpdateText(TEXT_ITEM_TIME, cViewerBuf);
}

//*****************************************************************************
//
// This function is called from the AcquireRun() function and should be in
// context of the main thread.  It pulls data items from the ADC data buffer,
// converts units as needed, and stores the results in a log record that is
// pointed at by the function parameter.
//
//*****************************************************************************
static void
ProcessDataItems(tLogRecord *pRecord)
{
    unsigned int uIdx;
    unsigned int uItemIdx = 0;
    unsigned long ulSelectedMask = g_pConfigState->usSelectedMask;

    //
    // Save the time stamp that was saved when the ADC data was acquired.
    // Also save into the record the bit mask of the selected data items.
    //
    pRecord->ulSeconds = g_ulTimeStamp[0];
    pRecord->usSubseconds = (unsigned short)g_ulTimeStamp[1];
    pRecord->usItemMask = (unsigned short)ulSelectedMask;

    //
    // Process the user analog input channels.  These will be converted and
    // stored as millivolts.
    //
    for(uIdx = LOG_ITEM_USER0; uIdx <= LOG_ITEM_USER3; uIdx++)
    {
        //
        // Check to see if this item should be logged
        //
        if((1 << uIdx) & ulSelectedMask)
        {
            unsigned long ulMillivolts;
            ulMillivolts = (g_ulADCData[uIdx] * 4100) / 819;
            pRecord->sItems[uItemIdx++] = (short)ulMillivolts;
        }
    }

    //
    // Process the accelerometers.  These will be processed and stored in
    // units of 1/100 g.
    //
    for(uIdx = LOG_ITEM_ACCELX; uIdx <= LOG_ITEM_ACCELZ; uIdx++)
    {
        //
        // Check to see if this item should be logged
        //
        if((1 << uIdx) & ulSelectedMask)
        {
            long lAccel;
            lAccel = (((long)g_ulADCData[uIdx] - 2047L) * 1000L) / 4095L;
            pRecord->sItems[uItemIdx++] = (short)lAccel;
        }
    }

    //
    // Process the external temperature. The temperature is stored in units
    // of 1/10 C.
    //
    if((1 << LOG_ITEM_EXTTEMP) & ulSelectedMask)
    {
        long lTempC;
        lTempC = (1866300 -
                  ((200000 * g_ulADCData[LOG_ITEM_EXTTEMP]) / 273)) / 1169;
        pRecord->sItems[uItemIdx++] = (short)lTempC;
    }

    //
    // Process the internal temperature. The temperature is stored in units
    // of 1/10 C.
    //
    if((1 << LOG_ITEM_INTTEMP) & ulSelectedMask)
    {
        long lTempC;
        lTempC = 1475 - ((2250 * g_ulADCData[LOG_ITEM_INTTEMP]) / 4095);
        pRecord->sItems[uItemIdx++] = (short)lTempC;
    }

    //
    // Process the current. The current is stored in units of 100 uA,
    // (or 1/10000 A).  Multiple current samples were taken in order
    // to average and smooth the data.
    //
    if((1 << LOG_ITEM_CURRENT) & ulSelectedMask)
    {
        unsigned long ulCurrent = 0;

        //
        // Average all the current samples that are available in the ADC
        // buffer.
        //
        for(uIdx = LOG_ITEM_CURRENT;
            uIdx < (LOG_ITEM_CURRENT + NUM_CURRENT_SAMPLES);
            uIdx++)
        {
            ulCurrent += g_ulADCData[uIdx];
        }
        ulCurrent /= NUM_CURRENT_SAMPLES;

        //
        // Convert the averaged current into units
        //
        ulCurrent = (ulCurrent * 200) / 273;
        pRecord->sItems[uItemIdx++] = (short)ulCurrent;
    }
}

//*****************************************************************************
//
// This is the handler for the ADC interrupt.  Even though more than one
// sequencer is used, they are configured so that this one runs last.
// Therefore when this ADC sequencer interrupt occurs, we know all of the ADC
// data has been acquired.
//
//*****************************************************************************
void
ADC0SS0Handler(void)
{
    //
    // Clear the interrupts for all ADC sequencers that are used.
    //
    ROM_ADCIntClear(ADC0_BASE, 0);
    ROM_ADCIntClear(ADC1_BASE, 0);

    //
    // Retrieve the data from all ADC sequencers
    //
    ROM_ADCSequenceDataGet(ADC0_BASE, 0, &g_ulADCData[0]);
    ROM_ADCSequenceDataGet(ADC1_BASE, 0, &g_ulADCData[8]);

    //
    // Set the time stamp, assume it is what was set for the last match
    // value.  This will be close to the actual time that the samples were
    // acquired, within a few microseconds.
    //
    g_ulTimeStamp[0] = g_ulNextMatch[0];
    g_ulTimeStamp[1] = g_ulNextMatch[1];

    //
    // Increment the ADC interrupt count
    //
    g_ulADCCount++;
}

//*****************************************************************************
//
// This is the handler for the RTC interrupt from the hibernate peripheral.
// It occurs on RTC match.  This handler will initiate an ADC acquisition,
// which will run all of the ADC sequencers.  Then it computes the next
// match value and sets it in the RTC.
//
//*****************************************************************************
void
RTCHandler(void)
{
    unsigned long ulStatus;
    unsigned long ulSeconds;

    //
    // Increment RTC interrupt counter
    //
    g_ulRTCInts++;

    //
    // Clear the RTC interrupts (this can be slow for hib module)
    //
    ulStatus = HibernateIntStatus(1);
    HibernateIntClear(ulStatus);

    //
    // Read and save the current value of the seconds counter.
    //
    ulSeconds = HibernateRTCGet();

    //
    // If we are sleep logging, then there will be no remembered value for
    // the next match value, which is also used as the time stamp when
    // data is collected.  In this case we will just use the current
    // RTC seconds.  This is safe because if sleep-logging is used, it
    // is only with periods of whole seconds, 1 second or longer.
    //
    if(g_pConfigState->ulSleepLogging)
    {
        g_ulNextMatch[0] = ulSeconds;
        g_ulNextMatch[1] = 0;
    }

    //
    // If we are logging data to PC and using a period greater than one
    // second, then use special handling.  For PC logging, if no data is
    // collected, then we must send a keep-alive packet once per second.
    //
    if((g_pConfigState->ucStorage == CONFIG_STORAGE_HOSTPC) &&
       (g_ulMatchPeriod[0] > 1))
    {
        //
        // If the current seconds count is less than the match value, that
        // means we got the interrupt due to one-second keep alive for the
        // host PC.
        //
        if(ulSeconds < g_ulNextMatch[0])
        {
            //
            // Set the next match for one second ahead (next keep-alive)
            //
            HibernateRTCMatch0Set(ulSeconds + 1);

            //
            // Set flag to indicate that a keep alive packet is needed
            //
            g_bNeedKeepAlive = 1;

            //
            // Nothing else to do except wait for next keep alive or match
            //
            return;
        }

        //
        // Else, this is a real match so proceed to below to do a normal
        // acquisition.
        //
    }

    //
    // Kick off the next ADC acquisition.  When these are done they will
    // cause an ADC interrupt.
    //
    ROM_ADCProcessorTrigger(ADC1_BASE, 0);
    ROM_ADCProcessorTrigger(ADC0_BASE, 0);

    //
    // Set the next RTC match.  Add the match period to the previous match
    // value.  We are making an assumption here that there is enough time
    // from when the match interrupt occurred, to this point in the code,
    // that we are still setting the match time in the future.  If the
    // period is too short, then we could miss a match and never get another
    // RTC interrupt.
    //
    g_ulNextMatch[0] += g_ulMatchPeriod[0];
    g_ulNextMatch[1] += g_ulMatchPeriod[1];
    if(g_ulNextMatch[1] > 32767)
    {
        //
        // Handle subseconds rollover
        //
        g_ulNextMatch[1] &= 32767;
        g_ulNextMatch[0]++;
    }

    //
    // If logging to host PC at greater than 1 second period, then set the
    // next RTC wakeup for 1 second from now.  This will cause a keep alive
    // packet to be sent to the PC
    //
    if((g_pConfigState->ucStorage == CONFIG_STORAGE_HOSTPC) &&
       (g_ulMatchPeriod[0] > 1))
    {
        HibernateRTCMatch0Set(ulSeconds + 1);
    }

    //
    // Otherwise this is a normal match and the next match should also be a
    // normal match, so set the next wakeup to the calculated match time.
    //
    else
    {
        HibernateRTCMatch0Set(g_ulNextMatch[0]);
        HibernateRTCSSMatch0Set(g_ulNextMatch[1]);
    }

    //
    // Toggle the LED on the board so the user can see that the acquisition
    // is running.
    //
    ROM_GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_2,
                     ~ROM_GPIOPinRead(GPIO_PORTG_BASE, GPIO_PIN_2));

    //
    // Now exit the int handler.  The ADC will trigger an interrupt when
    // it is finished, and the RTC is set up for the next match.
    //
}

//*****************************************************************************
//
// This function is called from the application main loop to keep the
// acquisition running.  It checks to see if there is any new ADC data, and
// if so it processes the new ADC data.
//
// The function returns non-zero if data was acquired, 0 if no data was
// acquired.
//
//*****************************************************************************
int
AcquireRun(void)
{
    static unsigned long ulLastADCCount = 0;
    tLogRecord *pRecord = &g_sRecordBuf.sRecord;

    //
    // Make sure we are properly configured to run
    //
    if(!g_pConfigState)
    {
        return(0);
    }

    //
    // Check to see if new ADC data is available
    //
    if(g_ulADCCount != ulLastADCCount)
    {
        ulLastADCCount = g_ulADCCount;

        //
        // Process the ADC data and store it in the record buffer.
        //
        ProcessDataItems(pRecord);

        //
        // Add the newly processed data to the strip chart.  Do not
        // add to strip start if sleep-logging.
        //
        if((g_pConfigState->ucStorage != CONFIG_STORAGE_VIEWER) &&
           !g_pConfigState->ulSleepLogging)
        {
            StripChartMgrAddItems(pRecord->sItems);
        }

        //
        // If USB stick is used, write the record to the USB stick
        //
        if(g_pConfigState->ucStorage == CONFIG_STORAGE_USB)
        {
            USBStickWriteRecord(pRecord);
        }

        //
        // If host PC is used, write data to USB serial port
        //
        if(g_pConfigState->ucStorage == CONFIG_STORAGE_HOSTPC)
        {
            USBSerialWriteRecord(pRecord);
        }

        //
        // If flash storage is used, write data to the flash
        //
        if(g_pConfigState->ucStorage == CONFIG_STORAGE_FLASH)
        {
            FlashStoreWriteRecord(pRecord);

            //
            // If we are sleep logging, then save the storage address for
            // use in the next cycle.
            //
            if(g_pConfigState->ulSleepLogging)
            {
                g_pConfigState->ulFlashStore = FlashStoreGetAddr();
            }
        }

        //
        // If in viewer mode, then update the viewer text strings.
        //
        else if(g_pConfigState->ucStorage == CONFIG_STORAGE_VIEWER)
        {
            UpdateViewerData(pRecord);
        }

        //
        // Return indication to caller that data was processed.
        //
        return(1);
    }

    //
    // Else there is no new data to process, but check to see if we are
    // logging to PC and a keep alive packet is needed.
    //
    else if((g_pConfigState->ucStorage == CONFIG_STORAGE_HOSTPC) &&
            g_bNeedKeepAlive)
    {
        //
        // Clear keep-alive needed flag
        //
        g_bNeedKeepAlive = 0;

        //
        // Make a keep alive packet by creating a record with timestamp of 0.
        //
        pRecord->ulSeconds = 0;
        pRecord->usSubseconds = 0;
        pRecord->usItemMask = 0;

        //
        // Transmit the dummy record to host PC.
        //
        USBSerialWriteRecord(pRecord);
    }

    //
    // Return indication that data was not processed.
    //
    return(0);
}

//*****************************************************************************
//
// This function is called to start an acquisition running.  It determines
// which channels are to be logged, enables the ADC sequencers, and computes
// the first RTC match value.  This will start the acquisition running.
//
//*****************************************************************************
void
AcquireStart(tConfigState *pConfig)
{
    unsigned long ulIdx;
    unsigned long ulRTC[2];
    unsigned long ulSelectedMask;

    //
    // Check the parameters
    //
    ASSERT(pConfig);
    if(!pConfig)
    {
        return;
    }

    //
    // Update the config state pointer, save the selected item mask
    //
    g_pConfigState = pConfig;
    ulSelectedMask = pConfig->usSelectedMask;

    //
    // Get the logging period from the logger configuration.  Split the
    // period into seconds and subseconds pieces and save for later use in
    // generating RTC match values.
    //
    g_ulMatchPeriod[0] = pConfig->ulPeriod >> 8;
    g_ulMatchPeriod[1] = (pConfig->ulPeriod & 0xFF) << 8;

    //
    // Determine how many channels are to be logged
    //
    ulIdx = ulSelectedMask;
    g_ulNumItems = 0;
    while(ulIdx)
    {
        if(ulIdx & 1)
        {
            g_ulNumItems++;
        }
        ulIdx >>= 1;
    }

    //
    // Initialize the strip chart manager for a new run.  Don't bother with
    // the strip chart if we are using viewer mode, or sleep-logging.
    //
    if((pConfig->ucStorage != CONFIG_STORAGE_VIEWER) &&
       !pConfig->ulSleepLogging)
    {
        StripChartMgrInit();
        StripChartMgrConfigure(ulSelectedMask);
    }

    //
    // Configure USB for memory stick if USB storage is chosen
    //
    if(pConfig->ucStorage == CONFIG_STORAGE_USB)
    {
        USBStickOpenLogFile(0);
    }

    //
    // If flash storage is to be used, prepare the flash storage module
    //
    else if(pConfig->ucStorage == CONFIG_STORAGE_FLASH)
    {
        //
        // If already sleep-logging, then pass in the saved flash address
        // so it does not need to be searched.
        //
        if(pConfig->ulSleepLogging)
        {
            FlashStoreOpenLogFile(pConfig->ulFlashStore);
        }

        //
        // Otherwise not sleep logging, so just initialize the flash store,
        // this will cause it to search for the starting storage address.
        //
        else
        {
            FlashStoreOpenLogFile(0);
        }
    }

    //
    // Enable the ADC sequencers
    //
    ROM_ADCSequenceEnable(ADC0_BASE, 0);
    ROM_ADCSequenceEnable(ADC1_BASE, 0);

    //
    // Flush the ADC sequencers to be sure there is no lingering data.
    //
    ROM_ADCSequenceDataGet(ADC0_BASE, 0, g_ulADCData);
    ROM_ADCSequenceDataGet(ADC1_BASE, 0, g_ulADCData);

    //
    // Enable ADC interrupts
    //
    ROM_ADCIntClear(ADC0_BASE, 0);
    ROM_ADCIntClear(ADC1_BASE, 0);
    ROM_ADCIntEnable(ADC0_BASE, 0);
    ROM_IntEnable(INT_ADC0SS0);

    //
    // If we are not already sleep-logging, then initialize the RTC match.
    // If we are sleep logging then this does not need to be set up.
    //
    if(!pConfig->ulSleepLogging)
    {
        //
        // Get the current RTC value
        //
        do
        {
            ulRTC[0] = HibernateRTCGet();
            ulRTC[1] = HibernateRTCSSGet();
        } while(ulRTC[0] != HibernateRTCGet());

        //
        // Set an initial next match value.  Start with the subseconds always
        // 0 so the first match value will always be an even multiple of the
        // subsecond match.  Add 2 seconds to the current RTC just to be clear
        // of an imminent rollover.  This means that the first match will occur
        // between 1 and 2 seconds from now.
        //
        g_ulNextMatch[0] = ulRTC[0] + 2;
        g_ulNextMatch[1] = 0;

        //
        // Now set the match value
        //
        HibernateRTCMatch0Set(g_ulNextMatch[0]);
        HibernateRTCSSMatch0Set(g_ulNextMatch[1]);
    }

    //
    // If we are configured to sleep, but not sleeping yet, then enter sleep
    // logging mode if allowed.
    //
    if(pConfig->bSleep && !pConfig->ulSleepLogging)
    {
        //
        // Allow sleep logging if storing to flash at a period of 1 second
        // or greater.
        //
        if((pConfig->ucStorage == CONFIG_STORAGE_FLASH) &&
           (pConfig->ulPeriod >= 0x100))
        {
            pConfig->ulSleepLogging = 1;
        }
    }

    //
    // Enable the RTC interrupts from the hibernate module
    //
    HibernateIntClear(HibernateIntStatus(0));
    HibernateIntEnable(HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_PIN_WAKE);
    ROM_IntEnable(INT_HIBERNATE);

    //
    // Logging data should now start running
    //
}

//*****************************************************************************
//
// This function is called to stop an acquisition running.  It disables the
// ADC sequencers and the RTC match interrupt.
//
//*****************************************************************************
void
AcquireStop(void)
{
    //
    // Disable RTC interrupts
    //
    ROM_IntDisable(INT_HIBERNATE);

    //
    // Disable ADC interrupts
    //
    ROM_IntDisable(INT_ADC0SS0);
    ROM_IntDisable(INT_ADC1SS0);

    //
    // Disable ADC sequencers
    //
    ROM_ADCSequenceDisable(ADC0_BASE, 0);
    ROM_ADCSequenceDisable(ADC1_BASE, 0);

    //
    // If USB stick is being used, then close the file so it will flush
    // the buffers to the USB stick.
    //
    if(g_pConfigState->ucStorage == CONFIG_STORAGE_USB)
    {
        USBStickCloseFile();
    }

    //
    // Disable the configuration pointer, which acts as a flag to indicate
    // if we are properly configured for data acquisition.
    //
    g_pConfigState = 0;
}

//*****************************************************************************
//
// This function initializes the ADC hardware in preparation for data
// acquisition.
//
//*****************************************************************************
void
AcquireInit(void)
{
    unsigned long ulChan;

    //
    // Enable the ADC peripherals and the associated GPIO port
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

    //
    // Enabled LED GPIO
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_2);

    //
    // Configure the pins to be used as analog inputs.
    //
    ROM_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                   GPIO_PIN_7 | GPIO_PIN_3);
    ROM_GPIOPinTypeADC(GPIO_PORTP_BASE, GPIO_PIN_0);

    //
    // Select the external reference for greatest accuracy.
    //
    ROM_ADCReferenceSet(ADC0_BASE, ADC_REF_EXT_3V);
    ROM_ADCReferenceSet(ADC1_BASE, ADC_REF_EXT_3V);

    //
    // Apply workaround for erratum 6.1, in order to use the
    // external reference.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    HWREG(GPIO_PORTB_BASE + GPIO_O_AMSEL) |= GPIO_PIN_6;

    //
    // Initialize both ADC peripherals using sequencer 0 and processor trigger.
    //
    ROM_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    ROM_ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Enter loop to configure all of the ADC sequencer steps needed to
    // acquire the data for the data logger.  Multiple ADC and sequencers
    // will be used in order to acquire all the channels.
    //
    for(ulChan = 0; ulChan < NUM_ADC_CHANNELS; ulChan++)
    {
        unsigned long ulBase;
        unsigned long ulSeq;
        unsigned long ulChCtl;

        //
        // If this is the first ADC then set the base for ADC0
        //
        if(ulChan < 8)
        {
            ulBase = ADC0_BASE;
            ulSeq = 0;
        }

        //
        // If this is the second ADC then set the base for ADC1
        //
        else if(ulChan < 16)
        {
            ulBase = ADC1_BASE;
            ulSeq = 0;
        }

        //
        // Get the channel control for each channel.  Test to see if it is
        // the last channel for the sequencer, and if so then also set the
        // interrupt and "end" flags.
        //
        ulChCtl = g_ulADCSeq[ulChan];
        if((ulChan == 7) || (ulChan == 15) ||
           (ulChan == (NUM_ADC_CHANNELS - 1)))
        {
            ulChCtl |= ADC_CTL_IE | ADC_CTL_END;
        }

        //
        // Configure the sequence step
        //
        ROM_ADCSequenceStepConfigure(ulBase, ulSeq, ulChan % 8, ulChCtl);
    }

    //
    // Erase the configuration in case there was a prior configuration.
    //
    g_pConfigState = 0;
}
