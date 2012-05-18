/***************************************************************
 *  SmartRF Studio(tm) Export
 *
 *  Radio register settings specifed with C-code
 *  compatible #define statements.
 *
 ***************************************************************/

#ifndef SMARTRF_CC1101_H
#define SMARTRF_CC1101_H

#if (!defined ISM_EU) && (!defined ISM_US) && (!defined ISM_LF)
#define ISM_US
#endif

#define SMARTRF_RADIO_CC1101

#ifdef ISM_EU
    // 869.50MHz
    #define SMARTRF_SETTING_FREQ2      0x21
    #define SMARTRF_SETTING_FREQ1      0x71
    #define SMARTRF_SETTING_FREQ0      0x7A
#else
  #ifdef ISM_US
    // 902MHz
    #define SMARTRF_SETTING_FREQ2      0x22
    #define SMARTRF_SETTING_FREQ1      0xB1
    #define SMARTRF_SETTING_FREQ0      0x3B
  #else
    #ifdef ISM_LF
        // 433.92MHz
        #define SMARTRF_SETTING_FREQ2      0x10
        #define SMARTRF_SETTING_FREQ1      0xB0
        #define SMARTRF_SETTING_FREQ0      0x71
    #else
        #error "Wrong ISM band specified (valid are ISM_LF, ISM_EU and ISM_US)"
    #endif // ISM_LF
  #endif // ISM_US
#endif // ISM_EU

#ifdef CHRONOS_RADIO_CONFIG
//
// Settings used when communicating with the CC430 in an
// eZ430 Chronos watch.
//
#define SMARTRF_SETTING_FSCTRL1    0x08
#define SMARTRF_SETTING_MDMCFG4    0x7B
#define SMARTRF_SETTING_MDMCFG3    0x83
#ifdef ISM_US
#define SMARTRF_SETTING_CHANNR     0x14
#else
#define SMARTRF_SETTING_CHANNR     0x00
#endif
#define SMARTRF_SETTING_DEVIATN    0x42
#define SMARTRF_SETTING_AGCCTRL0   0xB2
#define SMARTRF_SETTING_TEST2      0x81
#define SMARTRF_SETTING_TEST1      0x35
#define SMARTRF_SETTING_FIFOTHR    0x47
#else
//
// Default settings.
//
#define SMARTRF_SETTING_FSCTRL1    0x0C
#define SMARTRF_SETTING_MDMCFG4    0x2D
#define SMARTRF_SETTING_MDMCFG3    0x3B
#define SMARTRF_SETTING_CHANNR     0x14
#define SMARTRF_SETTING_DEVIATN    0x62
#define SMARTRF_SETTING_AGCCTRL0   0xB0
#define SMARTRF_SETTING_TEST2      0x88
#define SMARTRF_SETTING_TEST1      0x31
#define SMARTRF_SETTING_FIFOTHR    0x07
#endif

#define SMARTRF_SETTING_FSCTRL0    0x00
#define SMARTRF_SETTING_MDMCFG2    0x13
#define SMARTRF_SETTING_MDMCFG1    0x22
#define SMARTRF_SETTING_MDMCFG0    0xF8
#define SMARTRF_SETTING_FREND1     0xB6
#define SMARTRF_SETTING_FREND0     0x10
#define SMARTRF_SETTING_MCSM0      0x18
#define SMARTRF_SETTING_FOCCFG     0x1D
#define SMARTRF_SETTING_BSCFG      0x1C
#define SMARTRF_SETTING_AGCCTRL2   0xC7
#define SMARTRF_SETTING_AGCCTRL1   0x00
#define SMARTRF_SETTING_FSCAL3     0xEA
#define SMARTRF_SETTING_FSCAL2     0x2A
#define SMARTRF_SETTING_FSCAL1     0x00
#define SMARTRF_SETTING_FSCAL0     0x1F
#define SMARTRF_SETTING_FSTEST     0x59
#define SMARTRF_SETTING_TEST0      0x09
#define SMARTRF_SETTING_IOCFG2     0x29
#define SMARTRF_SETTING_IOCFG0D    0x06
#define SMARTRF_SETTING_PKTCTRL1   0x04
#define SMARTRF_SETTING_PKTCTRL0   0x05
#define SMARTRF_SETTING_ADDR       0x00
#define SMARTRF_SETTING_PKTLEN     0xFF

#endif

