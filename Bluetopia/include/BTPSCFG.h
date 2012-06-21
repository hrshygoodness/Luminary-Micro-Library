/*****< btpscfg.h >************************************************************/
/*      Copyright 2009 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSCFG - Stonestreet One Bluetooth Protocol Stack configuration          */
/*            directives/constants.  The information contained in this file   */
/*            controls various compile time parameters that are needed to     */
/*            build Bluetopia for a specific platform.                        */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/09/09  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BTPSCFGH__
#define __BTPSCFGH__

#include "BTPSKRNL.h"           /* BTPS Kernel Prototypes/Constants.          */

   /* Internal Timer Module Configuration.                              */
#define BTPS_CONFIGURATION_TIMER_MAXIMUM_NUMBER_CONCURRENT_TIMERS            10
#define BTPS_CONFIGURATION_TIMER_TIMER_THREAD_STACK_SIZE                    512
#define BTPS_CONFIGURATION_TIMER_MINIMUM_TIMER_RESOLUTION_MS                 50

   /* Generic HCI Driver Interface Configuration.                       */
#define BTPS_CONFIGURATION_HCI_DRIVER_RECEIVE_PACKET_BUFFER_SIZE           (HCI_CALCULATE_ACL_DATA_SIZE(1024))
#define BTPS_CONFIGURATION_HCI_DRIVER_BCSP_TIMER_THREAD_STACK_SIZE          256
#define BTPS_CONFIGURATION_HCI_DRIVER_BCSP_TIMER_MAXIMUM_CONCURRENT_TIMERS    8
#define BTPS_CONFIGURATION_HCI_DRIVER_BCSP_TIMER_MINIMUM_TIMER_RESOLUTION_MS 50
#define BTPS_CONFIGURATION_HCI_DRIVER_BCSP_UART_BUFFER_LENGTH              1024
#define BTPS_CONFIGURATION_HCI_DRIVER_BCSP_MAXIMUM_MESSAGE_LENGTH           512

   /* Host Controller Interface Configuration.                          */
#define BTPS_CONFIGURATION_HCI_MAXIMUM_SUPPORTED_HCI_PACKET_SIZE           1024
#define BTPS_CONFIGURATION_HCI_SYNCHRONOUS_WAIT_TIMEOUT_MS                 5000
#define BTPS_CONFIGURATION_HCI_DISPATCH_THREAD_STACK_SIZE                  3584
#define BTPS_CONFIGURATION_HCI_NUMBER_DISPATCH_MAILBOX_SLOTS                 46
#define BTPS_CONFIGURATION_HCI_DISPATCH_SCHEDULER_TIME_MS                  (BTPS_MINIMUM_SCHEDULER_RESOLUTION)

   /* L2CAP Configuration.                                              */
#define BTPS_CONFIGURATION_L2CAP_MAXIMUM_SUPPORTED_STACK_MTU               1017
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_RTX_TIMER_TIMEOUT_S                 15
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_ERTX_TIMER_TIMEOUT_S               300
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_IDLE_TIMER_TIMEOUT_S                 2
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_CONFIG_TIMER_TIMEOUT_S              60
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_RECEIVE_TIMER_TIMEOUT_S             60
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_LINK_CONNECT_REQUEST_CONFIG       (cqNoRoleSwitch)
#define BTPS_CONFIGURATION_L2CAP_DEFAULT_LINK_CONNECT_RESPONSE_CONFIG      (csMaintainCurrentRole)
#define BTPS_CONFIGURATION_L2CAP_ACL_CONNECTION_DELAY_TIMEOUT_MS              0

   /* RFCOMM Configuration.                                             */
#define BTPS_CONFIGURATION_RFCOMM_DEFAULT_LINK_TIMEOUT_MS                  (L2CAP_LINK_TIMEOUT_DEFAULT_VALUE)
#define BTPS_CONFIGURATION_RFCOMM_DEFAULT_ACKNOWLEDGEMENT_TIMER_S            20
#define BTPS_CONFIGURATION_RFCOMM_DEFAULT_RESPONSE_TIMER_S                   20
#define BTPS_CONFIGURATION_RFCOMM_EXTENDED_ACKNOWLEDGEMENT_TIMER_S           60
#define BTPS_CONFIGURATION_RFCOMM_CONNECTION_SUPERVISOR_TIMER_S              30
#define BTPS_CONFIGURATION_RFCOMM_DISCONNECT_SUPERVISOR_TIMER_S              30
#define BTPS_CONFIGURATION_RFCOMM_MAXIMUM_SUPPORTED_STACK_FRAME_SIZE      ((RFCOMM_FRAME_SIZE_MAXIMUM_VALUE > L2CAP_MAXIMUM_SUPPORTED_STACK_MTU)?L2CAP_MAXIMUM_SUPPORTED_STACK_MTU:RFCOMM_FRAME_SIZE_MAXIMUM_VALUE)
#define BTPS_CONFIGURATION_RFCOMM_DEFAULT_MAXIMUM_NUMBER_QUEUED_DATA_PACKETS  6
#define BTPS_CONFIGURATION_RFCOMM_DEFAULT_QUEUED_DATA_PACKETS_THRESHOLD       2

   /* SCO Configuration.                                                */
#define BTPS_CONFIGURATION_SCO_DEFAULT_BUFFER_SIZE                            0
#define BTPS_CONFIGURATION_SCO_DEFAULT_CONNECTION_MODE                    (scmEnableConnections)
#define BTPS_CONFIGURATION_SCO_DEFAULT_PHYSICAL_TRANSPORT                 (sptCodec)

   /* SDP Configuration.                                                */
#define BTPS_CONFIGURATION_SDP_PDU_RESPONSE_TIMEOUT_MS                    10000
#define BTPS_CONFIGURATION_SDP_DEFAULT_LINK_TIMEOUT_MS                    (L2CAP_LINK_TIMEOUT_DEFAULT_VALUE)
#define BTPS_CONFIGURATION_SDP_DEFAULT_DISCONNECT_MODE                    (dmAutomatic)

   /* SPP Configuration.                                                */
#define BTPS_CONFIGURATION_SPP_DEFAULT_SERVER_CONNECTION_MODE             (smAutomaticAccept)
#define BTPS_CONFIGURATION_SPP_MINIMUM_SUPPORTED_STACK_BUFFER_SIZE         1024
#define BTPS_CONFIGURATION_SPP_MAXIMUM_SUPPORTED_STACK_BUFFER_SIZE        65536
#define BTPS_CONFIGURATION_SPP_DEFAULT_TRANSMIT_BUFFER_SIZE                 256
#define BTPS_CONFIGURATION_SPP_DEFAULT_RECEIVE_BUFFER_SIZE                  256
#define BTPS_CONFIGURATION_SPP_DEFAULT_FRAME_SIZE                         ((128 > SPP_FRAME_SIZE_MAXIMUM)?SPP_FRAME_SIZE_MAXIMUM:128)

   /* OTP Configuration.                                                */
#define BTPS_CONFIGURATION_OTP_OBJECT_INFO_MAXIMUM_NAME_LENGTH              288
#define BTPS_CONFIGURATION_OTP_OBJECT_INFO_MAXIMUM_TYPE_LENGTH               64
#define BTPS_CONFIGURATION_OTP_OBJECT_INFO_MAXIMUM_OWNER_LENGTH              64
#define BTPS_CONFIGURATION_OTP_OBJECT_INFO_MAXIMUM_GROUP_LENGTH              64

   /* AVCTP Configuration.                                              */
#define BTPS_CONFIGURATION_AVCTP_DEFAULT_LINK_TIMEOUT                     (L2CAP_LINK_TIMEOUT_DEFAULT_VALUE)
#define BTPS_CONFIGURATION_AVCTP_MAXIMUM_SUPPORTED_MTU                    (L2CAP_MAXIMUM_SUPPORTED_STACK_MTU)
#define BTPS_CONFIGURATION_AVCTP_BROWSING_CHANNEL_SUPPORTED_DEFAULT       (FALSE)

#define BTPS_CONFIGURATION_AVCTP_SUPPORT_BROWSING_CHANNEL                             0

#define BTPS_CONFIGURATION_AVCTP_DEFAULT_BROWSING_CHANNEL_PDU_SIZE                 1024
#define BTPS_CONFIGURATION_AVCTP_DEFAULT_BROWSING_CHANNEL_TX_WINDOW                  10
#define BTPS_CONFIGURATION_AVCTP_DEFAULT_BROWSING_CHANNEL_MAX_TX_ATTEMPTS           255
#define BTPS_CONFIGURATION_AVCTP_DEFAULT_BROWSING_CHANNEL_MONITOR_TIMEOUT_MS       2000
#define BTPS_CONFIGURATION_AVCTP_DEFAULT_BROWSING_CHANNEL_RETRANSMISSION_TIMEOUT_MS 300

   /* AVRCP Configuration.                                              */
#define BTPS_CONFIGURATION_AVRCP_SUPPORT_BROWSING_CHANNEL                     (BTPS_CONFIGURATION_AVCTP_SUPPORT_BROWSING_CHANNEL)

   /* BIP Configuration.                                                */
#define BTPS_CONFIGURATION_BIP_MAXIMUM_OBEX_PACKET_LENGTH                  8000

   /* BPP Configuration.                                                */
#define BTPS_CONFIGURATION_BPP_MAXIMUM_OBEX_PACKET_LENGTH                  8000

   /* BTCOMM Configuration.                                             */
#define BTPS_CONFIGURATION_BTCOMM_COM_VCOM_BUFFER_SIZE                    (SPP_FRAME_SIZE_DEFAULT)
#define BTPS_CONFIGURATION_BTCOMM_SPP_TRANSMIT_BUFFER_SIZE                (SPP_BUFFER_SIZE_DEFAULT_TRANSMIT)
#define BTPS_CONFIGURATION_BTCOMM_SPP_RECEIVE_BUFFER_SIZE                 (SPP_BUFFER_SIZE_DEFAULT_RECEIVE)
#define BTPS_CONFIGURATION_BTCOMM_DISPATCH_THREAD_STACK_SIZE              65536
#define BTPS_CONFIGURATION_BTCOMM_NUMBER_DISPATCH_MAILBOX_SLOTS            1024

   /* BTSER Configuration.                                              */
#define BTPS_CONFIGURATION_BTSER_SER_VSER_BUFFER_SIZE                     (SPP_FRAME_SIZE_DEFAULT)
#define BTPS_CONFIGURATION_BTSER_SPP_TRANSMIT_BUFFER_SIZE                 (SPP_BUFFER_SIZE_DEFAULT_TRANSMIT)
#define BTPS_CONFIGURATION_BTSER_SPP_RECEIVE_BUFFER_SIZE                  (SPP_BUFFER_SIZE_DEFAULT_RECEIVE)
#define BTPS_CONFIGURATION_BTSER_DISPATCH_THREAD_STACK_SIZE               65536
#define BTPS_CONFIGURATION_BTSER_NUMBER_DISPATCH_MAILBOX_SLOTS             1024

   /* DUN Configuration.                                                */
#define BTPS_CONFIGURATION_DUN_SERIAL_BUFFER_SIZE                         (SPP_FRAME_SIZE_DEFAULT)
#define BTPS_CONFIGURATION_DUN_SPP_TRANSMIT_BUFFER_SIZE                   (SPP_BUFFER_SIZE_DEFAULT_TRANSMIT)
#define BTPS_CONFIGURATION_DUN_SPP_RECEIVE_BUFFER_SIZE                    (SPP_BUFFER_SIZE_DEFAULT_RECEIVE)
#define BTPS_CONFIGURATION_DUN_DISPATCH_THREAD_STACK_SIZE                 65536
#define BTPS_CONFIGURATION_DUN_NUMBER_DISPATCH_MAILBOX_SLOTS               1024

   /* FAX Configuration.                                                */
#define BTPS_CONFIGURATION_FAX_SERIAL_BUFFER_SIZE                         (SPP_FRAME_SIZE_DEFAULT)
#define BTPS_CONFIGURATION_FAX_SPP_TRANSMIT_BUFFER_SIZE                   (SPP_BUFFER_SIZE_DEFAULT_TRANSMIT)
#define BTPS_CONFIGURATION_FAX_SPP_RECEIVE_BUFFER_SIZE                    (SPP_BUFFER_SIZE_DEFAULT_RECEIVE)
#define BTPS_CONFIGURATION_FAX_DISPATCH_THREAD_STACK_SIZE                 65536
#define BTPS_CONFIGURATION_FAX_NUMBER_DISPATCH_MAILBOX_SLOTS               1024

   /* FTP Configuration.                                                */
#define BTPS_CONFIGURATION_FTP_MAXIMUM_OBEX_PACKET_LENGTH                  8000

   /* GAVD Configuration.                                               */
#define BTPS_CONFIGURATION_GAVD_DEFAULT_LINK_TIMEOUT_MS                   (L2CAP_LINK_TIMEOUT_DEFAULT_VALUE)
#define BTPS_CONFIGURATION_GAVD_SIGNALLING_RESPONSE_TIMEOUT_MS             6000
#define BTPS_CONFIGURATION_GAVD_SIGNALLING_DISCONNECT_TIMEOUT_MS           3000
#define BTPS_CONFIGURATION_GAVD_DATA_PACKET_QUEUEING_FLAGS                (L2CA_QUEUEING_FLAG_LIMIT_BY_PACKETS | L2CA_QUEUEING_FLAG_DISCARD_OLDEST)
#define BTPS_CONFIGURATION_GAVD_MAXIMUM_NUMBER_QUEUED_DATA_PACKETS           10
#define BTPS_CONFIGURATION_GAVD_QUEUED_DATA_PACKETS_THRESHOLD                 5

   /* HCRP Configuration.                                               */
#define BTPS_CONFIGURATION_HCRP_DEFAULT_LINK_TIMEOUT_MS                   10000

   /* HID Configuration.                                                */
#define HID_CONFIGURATION_HID_DEFAULT_LINK_TIMEOUT_MS                     10000

   /* LAP Configuration.                                                */
#define BTPS_CONFIGURATION_LAP_SERIAL_BUFFER_SIZE                         (SPP_FRAME_SIZE_DEFAULT)
#define BTPS_CONFIGURATION_LAP_SPP_TRANSMIT_BUFFER_SIZE                   (SPP_BUFFER_SIZE_DEFAULT_TRANSMIT)
#define BTPS_CONFIGURATION_LAP_SPP_RECEIVE_BUFFER_SIZE                    (SPP_BUFFER_SIZE_DEFAULT_RECEIVE)
#define BTPS_CONFIGURATION_LAP_DISPATCH_THREAD_STACK_SIZE                 65536
#define BTPS_CONFIGURATION_LAP_NUMBER_DISPATCH_MAILBOX_SLOTS               1024

   /* Object Push Configuration.                                        */
#define BTPS_CONFIGURATION_OBJP_MAXIMUM_OBEX_PACKET_LENGTH                 8000

   /* OPP Configuration.                                                */
#define BTPS_CONFIGURATION_OPP_MAXIMUM_OBEX_PACKET_LENGTH                  8000

   /* PAN Configuration.                                                */
#define BTPS_CONFIGURATION_PAN_DEFAULT_LINK_TIMEOUT_MS                    (L2CAP_LINK_TIMEOUT_DEFAULT_VALUE)
#define BTPS_CONFIGURATION_PAN_DEFAULT_CONTROL_PACKET_RESPONSE_TIMEOUT_MS 10000
#define BTPS_CONFIGURATION_PAN_DISPATCH_THREAD_STACK_SIZE                 32768
#define BTPS_CONFIGURATION_PAN_NUMBER_DISPATCH_MAILBOX_SLOTS               1024

   /* PBAP Configuration.                                               */
#define BTPS_CONFIGURATION_PBAP_MAXIMUM_OBEX_PACKET_LENGTH                 8000

   /* SYNC Configuration.                                               */
#define BTPS_CONFIGURATION_SYNC_MAXIMUM_OBEX_PACKET_LENGTH                 8000

   /* MAP Configuration.                                                */
#define BTPS_CONFIGURATION_MAP_MAXIMUM_MESSAGE_ACCESS_OBEX_PACKET_LENGTH    256
#define BTPS_CONFIGURATION_MAP_MAXIMUM_NOTIFICATION_OBEX_PACKET_LENGTH      256

#endif
