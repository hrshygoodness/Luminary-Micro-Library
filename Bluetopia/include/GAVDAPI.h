/*****< gavdapi.h >************************************************************/
/*      Copyright 2003 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  GAVDAPI - Stonestreet One Bluetooth Stack Generic Audio/Video Distribution*/
/*            Profile using the Audio/Video Distribution Transport Protocol   */
/*            (AVDTP) Type Definitions, Constants, and Prototypes.            */
/*                                                                            */
/*  Author:  David L. Hill                                                    */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname       Description of Modification                  */
/*   --------  -----------       ---------------------------------------------*/
/*   01/13/03  D. Hill           Initial creation.                            */
/******************************************************************************/
#ifndef __GAVDAPIH__
#define __GAVDAPIH__

#include "SS1BTPS.h"            /* Bluetooth Stack API Prototypes/Constants.  */

#include "GAVDType.h"           /* Bluetooth GAVD Type Definitions/Constants. */

   /* Error Return Codes.                                               */

   /* Error Codes that are smaller than these (less than -1000) are     */
   /* related to the Bluetooth Protocol Stack itself (see BTERRORS.H).  */
#define BTGAVD_ERROR_INVALID_PARAMETER                           (-1000)
#define BTGAVD_ERROR_NOT_INITIALIZED                             (-1001)
#define BTGAVD_ERROR_INVALID_BLUETOOTH_STACK_ID                  (-1002)
#define BTGAVD_ERROR_INSUFFICIENT_RESOURCES                      (-1004)
#define BTGAVD_ERROR_INVALID_RESPONSE                            (-1005)
#define BTGAVD_ERROR_INVALID_OPERATION                           (-1006)
#define BTGAVD_ERROR_L2CAP_MTU_EXCEEDED                          (-1007)
#define BTGAVD_ERROR_STREAMS_NOT_ON_SAME_DEVICE                  (-1008)

   /* The following constants represent the Status Values that are      */
   /* possible in the Connect Confirmation Event and the Open End Point */
   /* Confirmation Event.                                               */
#define GAVD_STATUS_SUCCESS                                         (0x00)
#define GAVD_STATUS_CONNECTION_TIMEOUT                              (0x01)
#define GAVD_STATUS_CONNECTION_REFUSED                              (0x02)
#define GAVD_STATUS_UNKNOWN_ERROR                                   (0x03)

   /* The following enumerated type represents the supported Server     */
   /* Connection Modes supported by the GAVD Server.                    */
typedef enum
{
   gsmAutomaticAccept,
   gsmAutomaticReject,
   gsmManualAccept
} GAVD_Server_Connection_Mode_t;

   /* The following constants represent the GAVD/AVDTP Defined Error    */
   /* Codes.                                                            */
#define GAVD_AVDTP_ERROR_SUCCESS                                    (0x00)

   /* Acceptor (ACP) to Initiator (INT), Signal Response Header         */
   /* Error Codes.                                                      */
   /* * NOTE * These are simply the AVDTP defined error codes.          */
#define GAVD_AVDTP_ERROR_BAD_HEADER_FORMAT                          (0x01)

   /* ACP to INT, Signal Response Payload Format Error Codes.           */
   /* * NOTE * These are simply the AVDTP defined error codes.          */
#define GAVD_AVDTP_ERROR_BAD_LENGTH                                 (0x11)
#define GAVD_AVDTP_ERROR_BAD_ACP_SEID                               (0x12)
#define GAVD_AVDTP_ERROR_SEP_IN_USE                                 (0x13)
#define GAVD_AVDTP_ERROR_SEP_NOT_IN_USE                             (0x14)
#define GAVD_AVDTP_ERROR_BAD_SERV_CATEGORY                          (0x17)
#define GAVD_AVDTP_ERROR_BAD_PAYLOAD_FORMAT                         (0x18)
#define GAVD_AVDTP_ERROR_NOT_SUPPORTED_COMMAND                      (0x19)
#define GAVD_AVDTP_ERROR_INVALID_CAPABILITIES                       (0x1A)

   /* ACP to INT, Signal Response Transport Service Capabilities Error  */
   /* Codes.                                                            */
   /* * NOTE * These are simply the AVDTP defined error codes.          */
#define GAVD_AVDTP_ERROR_BAD_RECOVERY_TYPE                          (0x22)
#define GAVD_AVDTP_ERROR_BAD_MEDIA_TRANSPORT_FORMAT                 (0x23)
#define GAVD_AVDTP_ERROR_BAD_RECOVERY_FORMAT                        (0x25)
#define GAVD_AVDTP_ERROR_BAD_ROHC_FORMAT                            (0x26)
#define GAVD_AVDTP_ERROR_BAD_CP_FORMAT                              (0x27)
#define GAVD_AVDTP_ERROR_BAD_MULTIPLEXING_FORMAT                    (0x28)
#define GAVD_AVDTP_ERROR_UNSUPPORTED_CONFIGURATION                  (0x29)

   /* ACP to INT, Procedure Error Codes.                                */
   /* * NOTE * These are simply the AVDTP defined error codes.          */
#define GAVD_AVDTP_ERROR_BAD_STATE                                  (0x31)
#define GAVD_AVDTP_ERROR_TIMEOUT                                    (0xFF)

   /* The following constants represent the maximum and minimum values  */
   /* that are possible for the MaxRecoveryWindowSize element in the    */
   /* structure GAVD_Recovery_Info_Element_Data_t.                      */
#define MAX_RECOVERY_WINDOW_SIZE_MIN_VALUE                          (0x01)
#define MAX_RECOVERY_WINDOW_SIZE_MAX_VALUE                          (0x18)

   /* The following constants represent the maximum and minimum values  */
   /* that are possible for the MaxNumberMediaPackets element in the    */
   /* structure GAVD_Recovery_Info_Element_Data_t.                      */
#define MAX_NUMBER_MEDIA_PACKETS_MIN_VALUE                          (0x01)
#define MAX_NUMBER_MEDIA_PACKETS_MAX_VALUE                          (0x18)

   /* GAVD Event API Types.                                             */

   /* These events are issued to the application via the callback       */
   /* registered when the application opens a local or remote GAVD      */
   /* device and/or a stream end point.                                 */
typedef enum
{
   etGAVD_Connect_Confirmation,
   etGAVD_Disconnect_Indication,
   etGAVD_Discover_Confirmation,
   etGAVD_Get_Capabilities_Confirmation,
   etGAVD_Get_Configuration_Confirmation,
   etGAVD_Set_Configuration_Indication,
   etGAVD_Open_End_Point_Indication,
   etGAVD_Open_End_Point_Confirmation,
   etGAVD_Close_End_Point_Indication,
   etGAVD_Start_Indication,
   etGAVD_Start_Confirmation,
   etGAVD_Suspend_Indication,
   etGAVD_Suspend_Confirmation,
   etGAVD_Reconfigure_Indication,
   etGAVD_Reconfigure_Confirmation,
   etGAVD_Security_Control_Indication,
   etGAVD_Security_Control_Confirmation,
   etGAVD_Abort_Indication,
   etGAVD_Abort_Confirmation,
   etGAVD_Data_Indication,
   etGAVD_Sender_Report_Data_Indication,
   etGAVD_Receiver_Report_Data_Indication,
   etGAVD_SDES_Report_Data_Indication,
   etGAVD_Recovery_Data_Indication,
   etGAVD_Data_Channel_Empty_Indication,
   etGAVD_Report_Data_Channel_Empty_Indication,
   etGAVD_Recovery_Data_Channel_Empty_Indication,
   etGAVD_Multiplexed_Channel_Empty_Indication,
   etGAVD_Connect_Request_Indication,
   etGAVD_Signalling_Connect_Indication,
   etGAVD_Signalling_Disconnect_Indication
} GAVD_Event_Type_t;

   /* The following enumerated type represents the available            */
   /* Types for Stream End Points.  These types are abbreviated as      */
   /* TSEPs by the protocol.                                            */
typedef enum
{
   tspSRC,
   tspSNK
} GAVD_TSEP_t;

   /* The following enumerated type represents the available            */
   /* Media Types for Stream End Points.                                */
typedef enum
{
   mtAudio,
   mtVideo,
   mtMultimedia
} GAVD_Media_Type_t;

   /* The following enumerated type represents the available            */
   /* Service Categories for Service Capabilities defined in the        */
   /* GAVD_Service_Capabilities_Info_t structure.                       */
typedef enum
{
   scNone,
   scMediaTransport,
   scReporting,
   scRecovery,
   scContentProtection,
   scHeaderCompression,
   scMultiplexing,
   scMediaCodec,
   scUnknown
} GAVD_Service_Category_t;

   /* The following enumerated type represents the available types of   */
   /* transport channels for GAVD/AVDTP Streams.                        */
typedef enum
{
   trMedia,
   trReporting,
   trRecovery,
   trNone
} GAVD_Transport_Channel_Type_t;

   /* The following structure is used with the                          */
   /* GAVD_Sender_Report_Data_Write() function.  This structure         */
   /* contains the Sender Information required to build the Sender      */
   /* Report RTCP packet.  The NTPTimeStampMSW member contains the most */
   /* significant word of the NTP time stamp.  The NTPTimeStampLSW      */
   /* member contains the least significant word of the NTP time stamp. */
   /* The RTPTimeStamp member contains the RTP time stamp.  The         */
   /* PacketCount member is the total number of packets this sender has */
   /* sent.  Finally, the OctetCount member is the total number of bytes*/
   /* that this sender has sent.                                        */
typedef struct _tagGAVD_Sender_Info_t
{
   DWord_t NTPTimeStampMSW;
   DWord_t NTPTimeStampLSW;
   DWord_t RTPTimeStamp;
   DWord_t PacketCount;
   DWord_t OctetCount;
} GAVD_Sender_Info_t;

#define GAVD_SENDER_INFO_SIZE                           (sizeof(GAVD_Sender_Info_t))

   /* The following structure is used with the                          */
   /* GAVD_Sender_Report_Data_Write() and                               */
   /* GAVD_Receiver_Report_Data_Write() functions.  This structure      */
   /* contains the Report Block information required to build the Sender*/
   /* Report (SR) or Receiver Report (RR) RTCP packet.                  */
typedef struct _tagGAVD_Report_Block_t
{
   DWord_t SourceID;
   Byte_t  FractionLost;
   DWord_t TotalPacketsLost;
   Word_t  HighestSeqNumReceivedCycleCount;
   Word_t  HighestSeqNumReceived;
   DWord_t IntervalJitter;
   DWord_t LastReport;
   DWord_t DelaySinceLastReport;
} GAVD_Report_Block_t;

#define GAVD_REPORT_BLOCK_SIZE                          (sizeof(GAVD_Report_Block_t))

   /* The following structure is used with the GAVD_SDES_Data_Write()   */
   /* function.  This structure contains the Source Description (SDES)  */
   /* Item information required to build the SDES RTCP packet.          */
typedef struct _tagGAVD_SDES_Item_t
{
   Byte_t  ItemDescriptor;
   Byte_t  ItemLength;
   Byte_t *ItemData;
} GAVD_SDES_Item_t;

#define GAVD_SDES_ITEM_SIZE                             (sizeof(GAVD_SDES_Item_t))

   /* The following structure is used with the GAVD_SDES_Data_Write()   */
   /* function.  This structure defines an SDES "Chunk" (see RFC 1889 p.*/
   /* 31) required to build the SDES RTCP packet.                       */
typedef struct _tagGAVD_SDES_Chunk_t
{
   DWord_t           SourceID;
   unsigned int      NumberSDESItems;
   GAVD_SDES_Item_t *SDESItems;
} GAVD_SDES_Chunk_t;

#define GAVD_SDES_CHUNK_SIZE                            (sizeof(GAVD_SDES_Chunk_t))

   /* The following structure is used with the                          */
   /* GAVD_Recovery_Data_Write() function.  This structure contains the */
   /* Forward Error Correction (FEC) information required to build the  */
   /* RTP FEC packet.                                                   */
typedef struct _tagGAVD_FEC_Block_t
{
   Word_t        SequenceNumberBase;
   Word_t        LengthRecovery;
   Byte_t        PayloadTypeRecovery;
   DWord_t       Mask;
   DWord_t       TimeStampRecovery;
   unsigned int  FECDataLength;
   Byte_t       *FECData;
} GAVD_FEC_Block_t;

#define GAVD_FEC_BLOCK_SIZE                             (sizeof(GAVD_FEC_Block_t))

   /* The following structure is used with the                          */
   /* GAVD_Register_SDP_Record() function.  This structure (when        */
   /* specified) contains additional SDP Service Information that will  */
   /* be added to the SDP GAVD Service Record Entry.  The first member  */
   /* of this structure specifies the Number of Service Class UUID's    */
   /* that are present in the SDPUUIDEntries Array.  This member must be*/
   /* at least one, and the SDPUUIDEntries member must point to an array*/
   /* of SDP UUID Entries that contains (at least) as many entries      */
   /* specified by the NumberServiceClassUUID member.  The ProtocolList */
   /* member is an SDP Data Element Sequence that contains a list of    */
   /* Protocol Information that will be added to the generic SDP Service*/
   /* Record.  The ProtocolList SDP Data Element must be a Data Element */
   /* Sequence containing the Protocol List information to add (in      */
   /* addition to the GAVD Protocol Information).  This element is      */
   /* optional and can be NULL (signifying no additional Protocol       */
   /* Information).  The ProfileList SDP Data Element must be a Data    */
   /* Element Sequence containing the Profile List information to add.  */
   /* This element is optional and can be NULL (signifying that no      */
   /* Profile Information is to be added).                              */
typedef struct _tagGAVD_SDP_Service_Record_t
{
   unsigned int        NumberServiceClassUUID;
   SDP_UUID_Entry_t   *SDPUUIDEntries;
   SDP_Data_Element_t *ProtocolList;
   SDP_Data_Element_t *ProfileList;
} GAVD_SDP_Service_Record_t;

#define GAVD_SDP_SERVICE_RECORD_SIZE                    (sizeof(GAVD_SDP_Service_Record_t))

   /* This structure defines the Recovery Information Element of the    */
   /* Recovery Service Capabilities that a stream endpoint may support. */
   /* The RecoveryType member indicates the recovery type used.  The    */
   /* MRWS member is the Maximum Recovery Window Size for a transport   */
   /* packet (0x01 to 0x18).  Finally, the MNMP member is the Maximum   */
   /* Number of Media Packets a specific parity code may cover (0x01 to */
   /* 0x18).                                                            */
typedef struct _tagGAVD_Recovery_Info_Element_Data_t
{
   Byte_t RecoveryType;
   Byte_t MaxRecoveryWindowSize;
   Byte_t MaxNumberMediaPackets;
} GAVD_Recovery_Info_Element_Data_t;

#define GAVD_RECOVERY_INFO_ELEMENT_DATA_SIZE            (sizeof(GAVD_Recovery_Info_Element_Data_t))

   /* This structure defines the Media Codec Information Element of the */
   /* Media Codec Service Capabilities that a stream endpoint may       */
   /* support.  The MediaType and MediaCodecType members are defined by */
   /* the Bluetooth Assigned Numbers.  The MediaCodecSpecificInfoLength */
   /* member is the number of bytes of specific information pointed to  */
   /* by the MediaTypeSpecificInfo element.  The MediaTypeSpecificInfo  */
   /* element points to a Byte array with information that is specific  */
   /* for each different type of Media Codec.                           */
typedef struct _tagGAVD_Media_Codec_Info_Element_Data_t
{
   GAVD_Media_Type_t  MediaType;
   Byte_t             MediaCodecType;
   Byte_t             MediaCodecSpecificInfoLength;
   Byte_t            *MediaCodecSpecificInfo;
} GAVD_Media_Codec_Info_Element_Data_t;

#define GAVD_MEDIA_CODEC_INFO_ELEMENT_DATA_SIZE         (sizeof(GAVD_Media_Codec_Info_Element_Data_t))

   /* This structure defines the Content Protection Information Element */
   /* of the Content Protection Service Capabilities that a stream      */
   /* endpoint may support.  The ContentProtectionType member specifies */
   /* the Content Protection Type of that data that follows.  These     */
   /* types are defined by the Bluetooth Assigned Numbers.  The         */
   /* ContentProtectionTypeSpecificInfoLength is the number of bytes of */
   /* specific information in the ContentProtectionTypeSpecificInfo     */
   /* member.  The ContentProtectionTypeSpecificInfo member points to a */
   /* Byte array containing information that is different for each      */
   /* different type of content protection.                             */
typedef struct _tagGAVD_Content_Protection_Info_Element_Data_t
{
   Word_t  ContentProtectionType;
   Byte_t  ContentProtectionTypeSpecificInfoLength;
   Byte_t *ContentProtectionTypeSpecificInfo;
} GAVD_Content_Protection_Info_Element_Data_t;

#define GAVD_CONTENT_PROTECTION_INFO_ELEMENT_DATA_SIZE  (sizeof(GAVD_Content_Protection_Info_Element_Data_t))

   /* This structure defines the Header Compression Information Element */
   /* of the Header Compression Service Capabilities that a stream      */
   /* endpoint may support.  The MediaPacketHeaderCompression member    */
   /* flags whether or not header compression is available for media    */
   /* packets.  The RecoveryPacketHeaderCompression member flags whether*/
   /* or not header compression is available for recovery packets.      */
   /* Finally, the BackChannelSupported member signals whether or not   */
   /* the header compression service supports a Back Channel.           */
typedef struct _tagGAVD_Header_Compression_Info_Element_Data_t
{
   Boolean_t MediaPacketHeaderCompression;
   Boolean_t RecoveryPacketHeaderCompression;
   Boolean_t BackChannelSupported;
} GAVD_Header_Compression_Info_Element_Data_t;

#define GAVD_HEADER_COMPRESSION_INFO_ELEMENT_DATA_SIZE  (sizeof(GAVD_Header_Compression_Info_Element_Data_t))

   /* This structure defines the Multiplexing Information Element of the*/
   /* Service Capabilities that a stream endpoint may support.          */
   /* AllowAdaptationLayerFragmentation specifies whether or not        */
   /* Fragmented Adaptation Layer packets are supported.  MediaTSID,    */
   /* ReportingTSID, and RecoveryTSID are used to assign a Transport    */
   /* Session ID (TSID) to the media, reporting, and recovery channels. */
   /* MediaTCID, ReportingTCID, and RecoveryTCID are used to assign a   */
   /* Transport Channel ID (TCID) to the media, reporting, and recovery */
   /* channels.  The TSIDs and TCIDs must match with the TSIDs and TCIDs*/
   /* of the channel that the user wishes to multiplex over.  If the    */
   /* user is registering a Stream End Point and wants to allow         */
   /* multiplexing but does not yet know the TCIDs and TSIDs, pass zeros*/
   /* in these parameters and allow a client to assign them when the    */
   /* client connects.  If the reporting and/or recovery channels are   */
   /* not used, pass zero for the reporting/recovery TCIDs and TSIDs.   */
typedef struct _tagGAVD_Multiplexing_Info_Element_Data_t
{
   unsigned int                  MediaMuxLSEID;
   GAVD_Transport_Channel_Type_t MediaMuxChannel;
   Boolean_t                     UseReportingChannel;
   unsigned int                  ReportingMuxLSEID;
   GAVD_Transport_Channel_Type_t ReportingMuxChannel;
   Boolean_t                     UseRecoveryChannel;
   unsigned int                  RecoveryMuxLSEID;
   GAVD_Transport_Channel_Type_t RecoveryMuxChannel;
} GAVD_Multiplexing_Info_Element_Data_t;

#define GAVD_MULTIPLEXING_INFO_ELEMENT_DATA_SIZE        (sizeof(GAVD_Multiplexing_Info_Element_Data_t))

   /* This structure defines a raw Information Element for generic use  */
   /* by the application.  This is an extension to the defined service  */
   /* categories.  The user should take great care when using this      */
   /* structure as this information will probably not be inter- operable*/
   /* with other implementations.  The RawDataLength member specifies   */
   /* the length of the data and the RawData member points to the data. */
typedef struct _tagGAVD_Raw_Info_Element_Data_t
{
   Byte_t  RawDataLength;
   Byte_t *RawData;
} GAVD_Raw_Info_Element_Data_t;

#define GAVD_RAW_INFO_ELEMENT_DATA_SIZE                 (sizeof(GAVD_Raw_Info_Element_Data_t))

   /* This structure defines the Service Capabilities that a stream     */
   /* endpoint supports.  The ServiceCategory member is the Capability's*/
   /* defined service category.  Finally, the InfoElement member        */
   /* contains specific Information Element data depending on the       */
   /* service category.                                                 */
   /* * NOTE * Some service categories do not have information elements.*/
   /*          For the service categories that do not have information  */
   /*          elements, this field is ignored.                         */
typedef struct _tagGAVD_Service_Capabilities_Info_t
{
   GAVD_Service_Category_t ServiceCategory;
   union
   {
      GAVD_Recovery_Info_Element_Data_t           GAVD_Recovery_Info_Element_Data;
      GAVD_Content_Protection_Info_Element_Data_t GAVD_Content_Protection_Info_Element_Data;
      GAVD_Header_Compression_Info_Element_Data_t GAVD_Header_Compression_Info_Element_Data;
      GAVD_Multiplexing_Info_Element_Data_t       GAVD_Multiplexing_Info_Element_Data;
      GAVD_Media_Codec_Info_Element_Data_t        GAVD_Media_Codec_Info_Element_Data;
      GAVD_Raw_Info_Element_Data_t                GAVD_Raw_Info_Element_Data;
   } InfoElement;
} GAVD_Service_Capabilities_Info_t;

#define GAVD_SERVICE_CAPABILITIES_INFO_SIZE             (sizeof(GAVD_Service_Capabilities_Info_t))

   /* The following structure is used with the                          */
   /* GAVD_Register_Stream_End_Point() and                              */
   /* GAVD_Open_Remote_Stream_End_Point() functions.  This structure    */
   /* contains the configuration information for the Local End Point.   */
   /* The TSEP member specifies whether this End Point is a Source or   */
   /* Sink.  The MediaType member identifies the media type (Audio,     */
   /* Video, or Multimedia) of this End Point.  The MediaInMTU,         */
   /* ReportingInMTU, and RecoveryInMTU members are the maximum input   */
   /* MTU sizes for the media transport, reporting, and recovery        */
   /* channels associated with this local stream end point              */
   /* (respectively).  The NumberCapabilities member specifies how many */
   /* Service Capability Information Elements are contained in an array */
   /* of Service Capabilities Information elements pointed to by the    */
   /* CapabilitiesInfo member.                                          */
typedef struct _tagGAVD_Local_End_Point_Info_t
{
   GAVD_TSEP_t                       TSEP;
   GAVD_Media_Type_t                 MediaType;
   Word_t                            MediaInMTU;
   Word_t                            ReportingInMTU;
   Word_t                            RecoveryInMTU;
   unsigned int                      NumberCapabilities;
   GAVD_Service_Capabilities_Info_t *CapabilitiesInfo;
} GAVD_Local_End_Point_Info_t;

#define GAVD_LOCAL_END_POINT_INFO_SIZE                  (sizeof(GAVD_Local_End_Point_Info_t))

   /* GAVD Events.                                                      */

   /* The following event is dispatched when a remote device is        */
   /* requesting a connection to the local device.  The BD_ADDR        */
   /* specifies the Bluetooth Address of the Remote Device that is      */
   /* connecting.                                                       */
   /* ** NOTE ** This event is only dispatched to servers that are in   */
   /*            Manual Accept Mode.                                    */
   /* ** NOTE ** This event must be responded to with the               */
   /*            GAVD_Connect_Request_Response() function in order to   */
   /*            accept or reject the outstanding Connect Request.      */
typedef struct _tagGAVD_Connect_Request_Indication_Data_t
{
   BD_ADDR_t BD_ADDR;
} GAVD_Connect_Request_Indication_Data_t;

#define GAVD_CONNECT_REQUEST_INDICATION_DATA_SIZE       (sizeof(GAVD_Connect_Request_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a local GAVD/AVDTP Client connects to a remote GAVD/AVDTP    */
   /* Stream End Point Manager.  GAVDID is the ID of the local GAVD     */
   /* Client.  Status indicates the success or failure (error code) of  */
   /* the connection request.                                           */
typedef struct _tagGAVD_Connect_Confirmation_Data_t
{
   unsigned int GAVDID;
   unsigned int Status;
   BD_ADDR_t    BD_ADDR;
} GAVD_Connect_Confirmation_Data_t;

#define GAVD_CONNECT_CONFIRMATION_DATA_SIZE             (sizeof(GAVD_Connect_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a local GAVD/AVDTP Client has been disconnected from a remote*/
   /* Stream End Point Manager.  GAVDID is the ID of the local GAVD     */
   /* Client.                                                           */
typedef struct _tagGAVD_Disconnect_Indication_Data_t
{
   unsigned int GAVDID;
   BD_ADDR_t    BD_ADDR;
} GAVD_Disconnect_Indication_Data_t;

#define GAVD_DISCONNECT_INDICATION_DATA_SIZE            (sizeof(GAVD_Disconnect_Indication_Data_t))

   /* This structure holds data about remote SEIDs that were discovered */
   /* using a GAVD AVDTP_DISCOVER Request and are contained in a GAVD   */
   /* AVDTP_DISCOVER Response.  The RSEID member is the ID of the Stream*/
   /* End Point on the remote side.  The TSEP member specifies the Type */
   /* (source or sink) of the Stream Endpoint.  The MediaType member    */
   /* specifies the media type of this Stream End Point.  Finally, the  */
   /* InUse member is a flag to tell if this Stream Endpoint is in use  */
   /* or not.                                                           */
typedef struct _tagGAVD_Remote_End_Point_Data_t
{
   unsigned int      RSEID;
   GAVD_TSEP_t       TSEP;
   GAVD_Media_Type_t MediaType;
   Boolean_t         InUse;
} GAVD_Remote_End_Point_Data_t;

#define GAVD_REMOTE_END_POINT_DATA_SIZE                 (sizeof(GAVD_Remote_End_Point_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to a  */
   /* GAVD/AVDTP Client to signal the completion of a "Stream Discover" */
   /* procedure requested by the GAVD/AVDTP Client.  The GAVDID is the  */
   /* ID of the local GAVD Client.  The ErrorCode member holds any      */
   /* errors that were reported.  The NumberRemoteEndPoints member is   */
   /* the number of remote stream end points that are contained in the  */
   /* array pointed to by the next element.  The RemoteEndPoints member */
   /* is a pointer to an array of stream end points that are available  */
   /* on the remote GAVD/AVDTP Stream End Point Manager.                */
typedef struct _tagGAVD_Discover_Confirmation_Data_t
{
   unsigned int                  GAVDID;
   unsigned int                  ErrorCode;
   unsigned int                  NumberRemoteEndPoints;
   GAVD_Remote_End_Point_Data_t *RemoteEndPoints;
} GAVD_Discover_Confirmation_Data_t;

#define GAVD_DISCOVER_CONFIRMATION_DATA_SIZE            (sizeof(GAVD_Discover_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to a  */
   /* GAVD/AVDTP Client to signal the completion of a "Stream Get       */
   /* Capabilities" procedure initiated by the GAVD/AVDTP Client.  The  */
   /* GAVDID member is the ID of the local GAVD server or client.  The  */
   /* RSEID member is the Remote Stream Endpoint ID (from the Client's  */
   /* point of view) that this Confirmation Event (response) holds the  */
   /* capabilities of.  The ErrorCode member contains any errors        */
   /* reported.  The NumberServiceCapabilities member specifies how many*/
   /* service capabilities elements are contained in the array pointed  */
   /* to by the next parameter.  Finally, the ServiceCapabilities member*/
   /* is a pointer to an array of Service Capabilities.                 */
typedef struct _tagGAVD_Get_Capabilities_Confirmation_Data_t
{
   unsigned int                      GAVDID;
   unsigned int                      RSEID;
   unsigned int                      ErrorCode;
   unsigned int                      NumberServiceCapabilities;
   GAVD_Service_Capabilities_Info_t *ServiceCapabilities;
} GAVD_Get_Capabilities_Confirmation_Data_t;

#define GAVD_GET_CAPABILITIES_CONFIRMATION_DATA_SIZE    (sizeof(GAVD_Get_Capabilities_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to a  */
   /* GAVD/AVDTP Client to signal the completion of a "Stream Get       */
   /* Configuration" procedure initiated by the GAVD/AVDTP Client.  The */
   /* GAVDID member is the ID of the local GAVD server or client.  The  */
   /* RSEID member is the Remote Stream Endpoint ID (from the Client's  */
   /* point of view) that this Confirmation Event (response) holds the  */
   /* capabilities of.  The ErrorCode member contains any errors        */
   /* reported.  The NumberServiceCapabilities member specifies how many*/
   /* service capabilities elements are contained in the array pointed  */
   /* to by the next parameter.  Finally, the ServiceCapabilities member*/
   /* is a pointer to an array of Service Capabilities.                 */
typedef struct _tagGAVD_Get_Configuration_Confirmation_Data_t
{
   unsigned int                      GAVDID;
   unsigned int                      RSEID;
   unsigned int                      ErrorCode;
   unsigned int                      NumberServiceCapabilities;
   GAVD_Service_Capabilities_Info_t *ServiceCapabilities;
} GAVD_Get_Configuration_Confirmation_Data_t;

#define GAVD_GET_CONFIGURATION_CONFIRMATION_DATA_SIZE   (sizeof(GAVD_Get_Configuration_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to the*/
   /* local application when it receives a Set Configuration request    */
   /* from a remote GAVD/AVDTP Client.  The BD_ADDR is the device       */
   /* address of the device from which the Set Configuration request    */
   /* originates.  The LSEID member is the ID of the Local Stream End   */
   /* Point that the remote Client wishes to set the configuration of.  */
   /* The NumberServiceCapabilities member specifies the number of      */
   /* Service Capabilities in the next element.  The                    */
   /* ServiceCapabilities member points to an array of Service          */
   /* Capabilities that the remote end wishes to set as the             */
   /* configuration for this Local Stream End Point.                    */
typedef struct _tagGAVD_Set_Configuration_Indication_Data_t
{
   BD_ADDR_t                         BD_ADDR;
   unsigned int                      LSEID;
   unsigned int                      NumberServiceCapabilities;
   GAVD_Service_Capabilities_Info_t *ServiceCapabilities;
} GAVD_Set_Configuration_Indication_Data_t;

#define GAVD_SET_CONFIGURATION_INDICATION_DATA_SIZE     (sizeof(GAVD_Set_Configuration_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to the*/
   /* local application when it receives an Open Stream request from a  */
   /* remote GAVD/AVDTP Client.  The LSEID member is the ID of the Local*/
   /* Stream End Point that the remote Client wishes to open.  The      */
   /* MediaOutMTU, ReportingOutMTU, and RecoveryOutMTU members specify  */
   /* the corresponding payload size (in bytes) of the corresponding    */
   /* Channel.                                                          */
typedef struct _tagGAVD_Open_End_Point_Indication_Data_t
{
   unsigned int LSEID;
   Word_t       MediaOutMTU;
   Word_t       ReportingOutMTU;
   Word_t       RecoveryOutMTU;
} GAVD_Open_End_Point_Indication_Data_t;

#define GAVD_OPEN_END_POINT_INDICATION_DATA_SIZE        (sizeof(GAVD_Open_End_Point_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a local GAVD/AVDTP Client receives a response to an Open     */
   /* request that the Client had previously sent to a Remote Stream End*/
   /* Point Manager.  The LSEID member is the ID of the (assumed) Local */
   /* Stream End Point that has (if successful) opened the Remote Stream*/
   /* End Point.  At this point, the LSEID is used as a handle to refer */
   /* to the stream opened between the local stream end point and the   */
   /* remote stream end point The ErrorCode member contains any AVDTP   */
   /* errors reported (0 if none).  The FirstFailingServiceCategory     */
   /* member holds the first failing Service Category in the            */
   /* Configuration that was requested in the Open request (scNone if   */
   /* none fail).  The MediaOutMTU, ReportingOutMTU, and RecoveryOutMTU */
   /* members specify the corresponding payload size (in bytes) of the  */
   /* corresponding configured channels.                                */
typedef struct _tagGAVD_Open_End_Point_Confirmation_Data_t
{
   unsigned int            LSEID;
   Word_t                  MediaOutMTU;
   Word_t                  ReportingOutMTU;
   Word_t                  RecoveryOutMTU;
   unsigned int            ErrorCode;
   GAVD_Service_Category_t FirstFailingServiceCategory;
} GAVD_Open_End_Point_Confirmation_Data_t;

#define GAVD_OPEN_END_POINT_CONFIRMATION_DATA_SIZE      (sizeof(GAVD_Open_End_Point_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP Stream End Point Manager to the*/
   /* local application when it receives a Close Stream request from a  */
   /* remote GAVD/AVDTP Client.  The LSEID member specifies the ID of   */
   /* the Local Stream End Point that the remote Client wishes to close.*/
typedef struct _tagGAVD_Close_End_Point_Indication_Data_t
{
   unsigned int LSEID;
} GAVD_Close_End_Point_Indication_Data_t;

#define GAVD_CLOSE_END_POINT_INDICATION_DATA_SIZE       (sizeof(GAVD_Close_End_Point_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a Local Stream End Point receives a "Stream Start" request   */
   /* from a Remote GAVD/AVDTP entity.  The LSEID member specifies the  */
   /* ID of the Local Stream End Point.                                 */
typedef struct _tagGAVD_Start_Indication_Data_t
{
   unsigned int LSEID;
} GAVD_Start_Indication_Data_t;

#define GAVD_START_INDICATION_DATA_SIZE                 (sizeof(GAVD_Start_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* in response to a Start Request that the local application had sent*/
   /* to the Remote GAVD/AVDTP entity.  The LSEID member specifies the  */
   /* ID of the Local Stream Endpoint.  The ErrorCode member contains   */
   /* the status of the Stream Start attempt (0 if successful).  The    */
   /* FirstFailingLSEID member is the SEID of the first Local Stream End*/
   /* Point that failed to start its Remote Stream End Point.  Remember */
   /* that the LSEID value is to be treated as a Stream Handle here.    */
typedef struct _tagGAVD_Start_Confirmation_Data_t
{
   unsigned int LSEID;
   unsigned int ErrorCode;
   unsigned int FirstFailingLSEID;
} GAVD_Start_Confirmation_Data_t;

#define GAVD_START_CONFIRMATION_DATA_SIZE               (sizeof(GAVD_Start_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when the GAVD/AVDTP entity receives a "Stream Suspend" request    */
   /* from the remote GAVD/AVDTP device.  The LSEID member specifies the*/
   /* Local Stream End Point ID (or, equivalently, the reference to the */
   /* stream) that is requested to be suspended.                        */
typedef struct _tagGAVD_Suspend_Indication_Data_t
{
   unsigned int LSEID;
} GAVD_Suspend_Indication_Data_t;

#define GAVD_SUSPEND_INDICATION_DATA_SIZE               (sizeof(GAVD_Suspend_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* in response to a Suspend Request that the local application had   */
   /* sent to the Remote GAVD/AVDTP entity.  The LSEID member specifies */
   /* the Local Stream Point ID.  The ErrorCode member contains the     */
   /* status of the Stream Suspend attempt (0 if successful).  The      */
   /* FirstFailingLSEID member is the SEID of the first Local Stream End*/
   /* Point that failed to suspend its Remote Stream End Point.         */
   /* Remember that the LSEID value is to be treated as a Stream Handle */
   /* here.                                                             */
typedef struct _tagGAVD_Suspend_Confirmation_Data_t
{
   unsigned int LSEID;
   unsigned int ErrorCode;
   unsigned int FirstFailingLSEID;
} GAVD_Suspend_Confirmation_Data_t;

#define GAVD_SUSPEND_CONFIRMATION_DATA_SIZE             (sizeof(GAVD_Suspend_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a remote GAVD/AVDTP device initiates a "Stream Reconfigure"  */
   /* procedure.  The LSEID member specifies the ID of the Local Stream */
   /* End Point that the remote device requested to reconfigure.  The   */
   /* NumberServiceCapabilities member is the number of service         */
   /* capabilities contained in the next element.  The                  */
   /* ServiceCapabilities member is a pointer to an array of service    */
   /* capabilities in the reconfigure request.                          */
   /* * NOTE * The ONLY valid Service Capabilities for this event are   */
   /*             (1) Media Codec Capabilities.                         */
   /*             (2) Content Protection Capabilities.                  */
   /*          The above Service Capabilities are the only Capabilities */
   /*          allowed to be reconfigured by AVDTP.                     */
typedef struct _tagGAVD_Reconfigure_Indication_Data_t
{
   unsigned int                      LSEID;
   unsigned int                      NumberServiceCapabilities;
   GAVD_Service_Capabilities_Info_t *ServiceCapabilities;
} GAVD_Reconfigure_Indication_Data_t;

#define GAVD_RECONFIGURE_INDICATION_DATA_SIZE           (sizeof(GAVD_Reconfigure_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* in response to a Stream Reconfigure request.  The LSEID member    */
   /* specifies the ID of the Local Stream End Point ID that is involved*/
   /* in the stream being reconfigured.  Remember that the LSEID is to  */
   /* be used as a handle to refer to the stream in this instance.  The */
   /* ErrorCode member is the status of the Reconfigure request (0 if   */
   /* successful).  Finally, the FirstFailingServiceCategory member is  */
   /* the first service category to fail in the reconfigure request     */
   /* (scNone if all succeed).                                          */
typedef struct _tagGAVD_Reconfigure_Confirmation_Data_t
{
   unsigned int            LSEID;
   unsigned int            ErrorCode;
   GAVD_Service_Category_t FirstFailingServiceCategory;
} GAVD_Reconfigure_Confirmation_Data_t;

#define GAVD_RECONFIGURE_CONFIRMATION_DATA_SIZE         (sizeof(GAVD_Reconfigure_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* to signal the reception of a GAVD AVDTP_SECURITY_CONTROL signal   */
   /* command from a peer entity.  The LSEID member specifies the ID of */
   /* the Local Stream End Point for which this Security Control change */
   /* is meant.  The SecurityDataLength member is the length of the     */
   /* content protection (security) data.  The SecurityData member is a */
   /* pointer to the Security Control data.                             */
typedef struct _tagGAVD_Security_Control_Indication_Data_t
{
   unsigned int  LSEID;
   unsigned int  SecurityDataLength;
   Byte_t       *SecurityData;
} GAVD_Security_Control_Indication_Data_t;

#define GAVD_SECURITY_CONTROL_INDICATION_DATA_SIZE      (sizeof(GAVD_Security_Control_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* to signal the completion of a "Security Control" procedure        */
   /* initiated by the local entity.  The LSEID member specifies the ID */
   /* of the Local Stream End Point (used to refer to the stream in this*/
   /* sense).  The ErrorCode member holds any errors reported.  The     */
   /* SecurityDataLength members is the length of the content protection*/
   /* (security) data.  Finally, the SecurityData member is a pointer to*/
   /* the content protection (security) data.                           */
typedef struct _tagGAVD_Security_Control_Confirmation_Data_t
{
   unsigned int  LSEID;
   unsigned int  ErrorCode;
   unsigned int  SecurityDataLength;
   Byte_t       *SecurityData;
} GAVD_Security_Control_Confirmation_Data_t;

#define GAVD_SECURITY_CONTROL_CONFIRMATION_DATA_SIZE    (sizeof(GAVD_Security_Control_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* to signal the reception of a GAVD AVDTP_ABORT signal command from */
   /* a peer entity.  The LSEID member specifies the ID of the Local    */
   /* Stream End Point for which this Abort command is meant.           */
typedef struct _tagGAVD_Abort_Indication_Data_t
{
   unsigned int LSEID;
} GAVD_Abort_Indication_Data_t;

#define GAVD_ABORT_INDICATION_DATA_SIZE                 (sizeof(GAVD_Abort_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* to signal the completion of an "Abort" procedure initiated by the */
   /* local entity.  The LSEID member specifies the ID of the Local     */
   /* Stream End Point that is attached to the Stream involved in the   */
   /* Abort procedure.  (Use the LSEID here to refer to the entire      */
   /* stream.)                                                          */
typedef struct _tagGAVD_Abort_Confirmation_Data_t
{
   unsigned int LSEID;
} GAVD_Abort_Confirmation_Data_t;

#define GAVD_ABORT_CONFIRMATION_DATA_SIZE               (sizeof(GAVD_Abort_Confirmation_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when Media Data is received.  The LSEID member is the Local Stream*/
   /* Endpoint for which this Data is meant.  The TimeStamp member      */
   /* contains timestamp information for the data.  The PayloadType     */
   /* member indicates the content type of the data.  The Marker member */
   /* specifies the marker that is used by certain codec types.  The    */
   /* DataLength member is the length of the received data.  Finally,   */
   /* the DataBuffer member is a pointer to the received data.  The     */
   /* Sequence number represents the sequence number of the data packet.*/
   /* This value should be incremented by one for each data packet      */
   /* received and can be used to determine packet loss and/or restore  */
   /* packet sequence.                                                  */
typedef struct _tagGAVD_Data_Indication_Data_t
{
   unsigned int  LSEID;
   DWord_t       TimeStamp;
   Byte_t        PayloadType;
   Boolean_t     Marker;
   Word_t        SequenceNumber;
   unsigned int  DataLength;
   Byte_t       *DataBuffer;
} GAVD_Data_Indication_Data_t;

#define GAVD_DATA_INDICATION_DATA_SIZE                  (sizeof(GAVD_Data_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when Sender Report (SR) Data is received.  The LSEID member is the*/
   /* Local Stream Endpoint for which this Data is meant.  The          */
   /* SenderInfo member contains the RTCP Sender Information.  The      */
   /* NumberReportBlocks member specifies how many report blocks are    */
   /* contained in the array of GAVD_Report_Block_t structures pointed  */
   /* to by the ReportBlocks member.  The ExtensionDataLength member is */
   /* the length (in DWord_t increments) of the received extension data.*/
   /* The ExtensionData member is a pointer to the received extension   */
   /* data.                                                             */
typedef struct _tagGAVD_Sender_Report_Data_Indication_Data_t
{
   unsigned int         LSEID;
   GAVD_Sender_Info_t  *SenderInfo;
   unsigned int         NumberReportBlocks;
   GAVD_Report_Block_t *ReportBlocks;
   unsigned int         ExtensionDataLength;
   DWord_t             *ExtensionData;
} GAVD_Sender_Report_Data_Indication_Data_t;

#define GAVD_SENDER_REPORT_DATA_INDICATION_DATA_SIZE    (sizeof(GAVD_Sender_Report_Data_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when Receiver Report (RR) Data is received.  The LSEID member     */
   /* specifies the Local Stream Endpoint for which this Data is meant. */
   /* The NumberReportBlocks member is how many report blocks are       */
   /* contained in an array of GAVD_Report_Block_t structures pointed   */
   /* to by the ReportBlocks member.  The ExtensionDataLength member is */
   /* the length (in DWord_t increments) of the received extension data.*/
   /* Finally, the ExtensionData member is a pointer to the received    */
   /* extension data.                                                   */
typedef struct _tagGAVD_Receiver_Report_Data_Indication_Data_t
{
   unsigned int         LSEID;
   unsigned int         NumberReportBlocks;
   GAVD_Report_Block_t *ReportBlocks;
   unsigned int         ExtensionDataLength;
   DWord_t             *ExtensionData;
} GAVD_Receiver_Report_Data_Indication_Data_t;

#define GAVD_RECEIVER_REPORT_DATA_INDICATION_DATA_SIZE  (sizeof(GAVD_Receiver_Report_Data_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when SDES Report Data is received.  The LSEID member specifies the*/
   /* Local Stream Endpoint for which this Data is meant.  The          */
   /* NumberSDESChunks member specifies how many SDES Chunks (items) are*/
   /* contained in an array of GAVD_SDES_Item_t structures pointed to by*/
   /* the SDESItems member.                                             */
typedef struct _tagGAVD_SDES_Report_Data_Indication_Data_t
{
   unsigned int       LSEID;
   unsigned int       NumberSDESChunks;
   GAVD_SDES_Chunk_t *SDESChunks;
} GAVD_SDES_Report_Data_Indication_Data_t;

#define GAVD_SDES_REPORT_DATA_INDICATION_DATA_SIZE      (sizeof(GAVD_SDES_Report_Data_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when FEC Recovery Data is received.  The LSEID member specifies   */
   /* the Local Stream Endpoint for which this Data is meant.  The      */
   /* TimeStamp member contains timestamp information for the data.  The*/
   /* PayloadType member indicates the content type of the data.  The   */
   /* Marker member is a marker that is used by certain codec types.    */
   /* The FECBlock member is a pointer to a GAVD_FEC_Block_t structure  */
   /* that contains the FEC Block information.  The FECDataLength member*/
   /* specifies the length (in DWord_t increments) of the received FEC  */
   /* data.  Finally, the FECData member is a pointer to the data.      */
typedef struct _tagGAVD_Recovery_Data_Indication_Data_t
{
   unsigned int      LSEID;
   DWord_t           TimeStamp;
   Byte_t            PayloadType;
   Boolean_t         Marker;
   Word_t            SequenceNumber;
   GAVD_FEC_Block_t *FECBlock;
} GAVD_Recovery_Data_Indication_Data_t;

#define GAVD_RECOVERY_DATA_INDICATION_DATA_SIZE         (sizeof(GAVD_Recovery_Data_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a Stream End Point no longer has any data queued to be sent  */
   /* (on the Data Channel).  The LSEID member represents the Local     */
   /* Stream Endpoint that now has an empty Data Channel.               */
   /* * NOTE * This event is only dispatched when the Data Write        */
   /*          function returned the error code                         */
   /*                                                                   */
   /*             BTPS_ERROR_INSUFFICIENT_RESOURCES                     */
   /*                                                                   */
   /*          If the return value of the Data Write function is not    */
   /*          the above mentioned error then this event will not be    */
   /*          dispatched.                                              */
   /* * NOTE * This is a generic structure that is used for all Data    */
   /*          Channels.  The Event type will specify what type of Data */
   /*          Channel (for the specified Stream Endpoint) now has empty*/
   /*          Buffers.                                                 */
typedef struct _tagGAVD_Channel_Empty_Indication_Data_t
{
   unsigned int LSEID;
} GAVD_Channel_Empty_Indication_Data_t;

#define GAVD_CHANNEL_EMPTY_INDICATION_DATA_SIZE         (sizeof(GAVD_Channel_Empty_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a Signalling Channel has been connected (ONLY when the local */
   /* application has registered for these events via the               */
   /* GAVD_Register_Signalling_Connection_Status() function).           */
typedef struct _tagGAVD_Signalling_Connect_Indication_Data_t
{
   unsigned int GAVDConnectionStatusID;
   BD_ADDR_t    BD_ADDR;
} GAVD_Signalling_Connect_Indication_Data_t;

#define GAVD_SIGNALLING_CONNECT_INDICATION_DATA_SIZE    (sizeof(GAVD_Signalling_Connect_Indication_Data_t))

   /* This event is sent by a GAVD/AVDTP entity to the local application*/
   /* when a Signalling Channel has been disconnected (ONLY when the    */
   /* local application has registered for these events via the         */
   /* GAVD_Register_Signalling_Connection_Status() function).           */
typedef struct _tagGAVD_Signalling_Disconnect_Indication_Data_t
{
   unsigned int GAVDConnectionStatusID;
   BD_ADDR_t    BD_ADDR;
} GAVD_Signalling_Disconnect_Indication_Data_t;

#define GAVD_SIGNALLING_DISCONNECT_INDICATION_DATA_SIZE (sizeof(GAVD_Signalling_Disconnect_Indication_Data_t))

   /* The following structure represents the container structure for    */
   /* Holding all GAVD Event Data Data.                                 */
typedef struct _tagGAVD_Event_Data_t
{
   GAVD_Event_Type_t Event_Data_Type;
   Word_t            Event_Data_Size;
   union
   {
      GAVD_Connect_Request_Indication_Data_t       *GAVD_Connect_Request_Indication_Data;
      GAVD_Connect_Confirmation_Data_t             *GAVD_Connect_Confirmation_Data;
      GAVD_Disconnect_Indication_Data_t            *GAVD_Disconnect_Indication_Data;
      GAVD_Discover_Confirmation_Data_t            *GAVD_Discover_Confirmation_Data;
      GAVD_Get_Capabilities_Confirmation_Data_t    *GAVD_Get_Capabilities_Confirmation_Data;
      GAVD_Get_Configuration_Confirmation_Data_t   *GAVD_Get_Configuration_Confirmation_Data;
      GAVD_Set_Configuration_Indication_Data_t     *GAVD_Set_Configuration_Indication_Data;
      GAVD_Open_End_Point_Indication_Data_t        *GAVD_Open_End_Point_Indication_Data;
      GAVD_Open_End_Point_Confirmation_Data_t      *GAVD_Open_End_Point_Confirmation_Data;
      GAVD_Close_End_Point_Indication_Data_t       *GAVD_Close_End_Point_Indication_Data;
      GAVD_Start_Indication_Data_t                 *GAVD_Start_Indication_Data;
      GAVD_Start_Confirmation_Data_t               *GAVD_Start_Confirmation_Data;
      GAVD_Suspend_Indication_Data_t               *GAVD_Suspend_Indication_Data;
      GAVD_Suspend_Confirmation_Data_t             *GAVD_Suspend_Confirmation_Data;
      GAVD_Reconfigure_Indication_Data_t           *GAVD_Reconfigure_Indication_Data;
      GAVD_Reconfigure_Confirmation_Data_t         *GAVD_Reconfigure_Confirmation_Data;
      GAVD_Security_Control_Indication_Data_t      *GAVD_Security_Control_Indication_Data;
      GAVD_Security_Control_Confirmation_Data_t    *GAVD_Security_Control_Confirmation_Data;
      GAVD_Abort_Indication_Data_t                 *GAVD_Abort_Indication_Data;
      GAVD_Abort_Confirmation_Data_t               *GAVD_Abort_Confirmation_Data;
      GAVD_Data_Indication_Data_t                  *GAVD_Data_Indication_Data;
      GAVD_Sender_Report_Data_Indication_Data_t    *GAVD_Sender_Report_Data_Indication_Data;
      GAVD_Receiver_Report_Data_Indication_Data_t  *GAVD_Receiver_Report_Data_Indication_Data;
      GAVD_SDES_Report_Data_Indication_Data_t      *GAVD_SDES_Report_Data_Indication_Data;
      GAVD_Recovery_Data_Indication_Data_t         *GAVD_Recovery_Data_Indication_Data;
      GAVD_Channel_Empty_Indication_Data_t         *GAVD_Channel_Empty_Indication_Data;
      GAVD_Signalling_Connect_Indication_Data_t    *GAVD_Signalling_Connect_Indication_Data;
      GAVD_Signalling_Disconnect_Indication_Data_t *GAVD_Signalling_Disconnect_Indication_Data;
   } Event_Data;
} GAVD_Event_Data_t;

#define GAVD_EVENT_DATA_SIZE                            (sizeof(GAVD_Event_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* a GAVD Event Receive Data Callback.  This function will be        */
   /* called whenever a GAVD Event occurs that is associated with the   */
   /* specified Bluetooth Stack ID.  This function passes to the caller */
   /* the Bluetooth Stack ID, the GAVD Event Data that occurred and     */
   /* the GAVD Event Callback Parameter that was specified when this    */
   /* Callback was installed.  The caller is free to use the contents   */
   /* of the GAVD Event Data ONLY in the context of this callback.  If  */
   /* the caller requires the Data for a longer period of time, then    */
   /* the callback function MUST copy the data into another Data        */
   /* Buffer.  This function is guaranteed NOT to be invoked more than  */
   /* once simultaneously for the specified installed callback (i.e.    */
   /* this function DOES NOT have be reentrant).  It needs to be noted, */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the       */
   /* Thread Context of a Thread that the User does NOT own.  Therefore,*/
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another GAVD Event will not   */
   /* be processed while this function call is outstanding).            */
   /* ** NOTE ** This function MUST NOT block and wait for events that  */
   /*            can only be satisfied by Receiving GAVD Events.  A     */
   /*            deadlock WILL occur because NO GAVD Event Callbacks    */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
typedef void (BTPSAPI *GAVD_Event_Callback_t)(unsigned int BluetoothStackID, GAVD_Event_Data_t *GAVD_Event_Data, unsigned long CallbackParameter);

   /* The following function is responsible for initializing the GAVD   */
   /* Profile.  This routine initializes and starts a local Stream End  */
   /* Point Manager.  This function MUST be called before any other GAVD*/
   /* Profile functions.  This function accepts as input the Bluetooth  */
   /* Stack Instance ID to use for the GAVD entity.                     */
   /* ** NOTE ** This function may only be called once for a given      */
   /*            Bluetooth Stack ID.                                    */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Initialize(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Initialize_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is responsible for removing a GAVD Profile */
   /* from the system.  Call this routine to completely remove the GAVD */
   /* Profile and shut down the local Stream End Point Manager.  After  */
   /* calling this function, GAVD_Initialize() must be called again     */
   /* before any other GAVD Profile functions can be called.  This      */
   /* function accepts the Bluetooth Stack ID that this GAVD Profile is */
   /* valid for.                                                        */
BTPSAPI_DECLARATION void BTPSAPI GAVD_Cleanup(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_GAVD_Cleanup_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is responsible for responding to an        */
   /* individual request to connect to a local GAVD/AVDTP Server.  The  */
   /* first parameter to this function is the Bluetooth Stack ID of the */
   /* Bluetooth Stack associated with the GAVD/AVDTP Server that is     */
   /* responding to the request.  The second parameter to this function */
   /* is the Bluetooth Device Address of the GAVD Connection for which a*/
   /* connection request was received.  The final parameter to this     */
   /* function specifies whether to accept the pending connection       */
   /* request (or to reject the request).  This function returns zero if*/
   /* successful, or a negative return error code if an error occurred. */
   /* ** NOTE ** The connection to the server is not established until  */
   /*            a Connect Indication event has actually occurred.      */
   /* ** NOTE ** This event will only be dispatched if the server mode  */
   /*            was explicitly set to gsmManualAccept via the          */
   /*            GAVD_Set_Server_Connection_Mode() function.  If this   */
   /*            mode is set, ONLY the callback that was specified with */
   /*            GAVD_Initialize() function will receive this event.    */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Connect_Request_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Boolean_t AcceptConnection);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Connect_Request_Response_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Boolean_t AcceptConnection);
#endif

   /* The following function is provided to allow a means to add a      */
   /* Generic SDP Service Record to the SDP Database.  This function    */
   /* takes as input the Bluetooth Stack ID of the Local Bluetooth      */
   /* Protocol Stack.  The next parameter (if specified) specifies any  */
   /* additional SDP Information to add to the record.  The third       */
   /* parameter specifies the Service Name to associate with the SDP    */
   /* Record.  The final parameter is a pointer to a DWord_t which      */
   /* receives the SDP Service Record Handle if this function           */
   /* successfully creates an SDP Service Record.  If this function     */
   /* returns zero, then the SDPServiceRecordHandle entry will contain  */
   /* the Service Record Handle of the added SDP Service Record.  If    */
   /* this function fails, a negative return error code will be         */
   /* returned (see BTERRORS.H)                                         */
   /* and the SDPServiceRecordHandle value will be undefined.           */
   /* ** NOTE ** The Service Record Handle that is returned from this   */
   /*            function will remain in the SDP Record Database until  */
   /*            it is deleted by calling the                           */
   /*            SDP_Delete_Service_Record() function.                  */
   /* ** NOTE ** A MACRO is provided to Delete the Service Record from  */
   /*            the SDP Data Base.  This MACRO maps the                */
   /*            GAVD_Un_Register_SDP_Record() to                       */
   /*            SDP_Delete_Service_Record().                           */
   /* ** NOTE ** Any Protocol Information that is specified will be     */
   /*            added in the Protocol Attribute AFTER the default      */
   /*            Protocol List (L2CAP and AVDTP).                       */
   /* ** NOTE ** The Service Name is always added at Attribute ID       */
   /*            0x0100.  A Language Base Attribute ID List is created  */
   /*            that specifies that 0x0100 is UTF-8 Encoded, English   */
   /*            Language.                                              */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Register_SDP_Record(unsigned int BluetoothStackID, GAVD_SDP_Service_Record_t *SDPServiceRecord, char *ServiceName, DWord_t *SDPServiceRecordHandle);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Register_SDP_Record_t)(unsigned int BluetoothStackID, GAVD_SDP_Service_Record_t *SDPServiceRecord, char *ServiceName, DWord_t *SDPServiceRecordHandle);
#endif

   /* The following MACRO is a utility MACRO that simply deletes the    */
   /* GAVD SDP Service Record (specified by the next parameter) from    */
   /* SDP Database.  This MACRO simply maps to the                      */
   /* SDP_Delete_Service_Record() function.  This MACRO is only         */
   /* provided so that the caller doesn't have to sift through the SDP  */
   /* API for very simplistic applications.  This function accepts as   */
   /* input the Bluetooth Stack ID of the Bluetooth Protocol Stack that */
   /* the Service Record exists on and the SDP Service Record Handle.   */
   /* The SDP Service Record Handle was returned via a successful call  */
   /* to the GAVD_Register_SDP_Record() function.  See the              */
   /* GAVD_Register_SDP_Record() function for more information.  This   */
   /* MACRO returns the result of the SDP_Delete_Service_Record()       */
   /* function, which is zero for success or a negative return error    */
   /* code (see BTERRORS.H).                                            */
#define GAVD_Un_Register_SDP_Record(__BluetoothStackID, __SDPRecordHandle) \
        (SDP_Delete_Service_Record(__BluetoothStackID, __SDPRecordHandle))

   /* The following function is used by a local GAVD/AVDTP Client to    */
   /* connect to a remote Stream End Point Manager.  This function      */
   /* accepts as input, the Bluetooth Stack ID of the Bluetooth Protocol*/
   /* Stack that this local GAVD/AVDTP Client is associated with.  The  */
   /* second parameter is the Bluetooth Device Address of the Remote    */
   /* AVDTP Stream End Point Manager to connect to.  The final two      */
   /* parameters specify the GAVD Event Callback Function and the       */
   /* Callback Parameter to associate with this GAVD/AVDTP Client.  This*/
   /* function returns a positive, non-zero, value if successful or a   */
   /* negative return error code if an error occurs.  A successful      */
   /* return code will be a GAVDID that can be used to reference the    */
   /* opened GAVD Client in ALL other GAVD Client functions in this     */
   /* module.  Once a connection to a remote server is established, the */
   /* connection may be closed by calling GAVD_Disconnect() (passing the*/
   /* return value from this function).                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Connect(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Connect_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is responsible for closing a connection to */
   /* a Remote GAVD/AVDTP Stream End Point Manager which was opened by a*/
   /* successful call to GAVD_Connect().  This function accepts as      */
   /* input, the Bluetooth Stack ID of the Bluetooth Protocol Stack that*/
   /* the GAVD/AVDTP Client (specified by the second parameter) is valid*/
   /* for.  This function returns zero if successful or a negative      */
   /* return error code if an error occurred (see BTERRORS.H).  Note    */
   /* that this function DOES NOT CLOSE the remote Stream End Point     */
   /* Manager.  This function merely closes this Client's connection to */
   /* the remote Stream End Point Manager IF and ONLY IF no Stream End  */
   /* Points were established.  If Stream End Points were established,  */
   /* calling this routine will NOT close the physical connection.  The */
   /* user must close all end points before calling this routine if the */
   /* user wished to close the physical connection.                     */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Disconnect(unsigned int BluetoothStackID, unsigned int GAVDID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Disconnect_t)(unsigned int BluetoothStackID, unsigned int GAVDID);
#endif

   /* The following function is responsible for sending a "Discover     */
   /* Request" to a remote Stream End Point Manager.  This function     */
   /* accepts as input, the Bluetooth Stack ID of the Bluetooth Stack   */
   /* Instance and the GAVDID of the GAVD/AVDTP Client that was returned*/
   /* after a call to GAVD_Connect().  This function returns zero on    */
   /* success or a negative return error code on failure.               */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Discover_End_Points(unsigned int BluetoothStackID, unsigned int GAVDID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Discover_End_Points_t)(unsigned int BluetoothStackID, unsigned int GAVDID);
#endif

   /* The following function is responsible for sending a "Get          */
   /* Capabilities Request" to a remote Stream End Point Manager.  This */
   /* function accepts as input, the Bluetooth Stack ID of the local    */
   /* Bluetooth Protocol Stack, the GAVDID of the local GAVD/AVDTP      */
   /* Client, and the Remote Stream Endpoint ID (RSEID) of the Stream   */
   /* Endpoint the Client wishes to get the capabilities of.  This RSEID*/
   /* was returned in a Discover Confirmation Event after a successful  */
   /* discovery of the Stream End Point Manager's registered Stream End */
   /* Points.  This function returns zero if successful, or a negative  */
   /* return error code if an error occurred (see BTERRORS.H).          */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Get_End_Point_Capabilities(unsigned int BluetoothStackID, unsigned int GAVDID, unsigned int RSEID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Get_End_Point_Capabilities_t)(unsigned int BluetoothStackID, unsigned int GAVDID, unsigned int RSEID);
#endif

   /* The following function is responsible for sending a "Get          */
   /* Configuration Request" to a remote Stream End Point Manager.  This*/
   /* function accepts as input, the Bluetooth Stack ID of the local    */
   /* Bluetooth Protocol Stack, the GAVDID of the local GAVD/AVDTP      */
   /* Client, and the Remote Stream Endpoint ID (RSEID) of the Stream   */
   /* Endpoint the Client wishes to get the configuration of.  This     */
   /* RSEID was returned in a Discover Confirmation Event after a       */
   /* successful discovery of the Stream End Point Manager's registered */
   /* Stream End Points.  This function returns zero if successful, or a*/
   /* negative return error code if an error occurred (see BTERRORS.H). */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Get_Configuration(unsigned int BluetoothStackID, unsigned int GAVDID, unsigned int RSEID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Get_Configuration_t)(unsigned int BluetoothStackID, unsigned int GAVDID, unsigned int RSEID);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* register Local Stream End Points with the local Stream End Point  */
   /* Manager.  This function accepts as input, the Bluetooth Stack ID  */
   /* of the Bluetooth Protocol Stack that the Stream End Point Manager */
   /* is valid for.  LocalEndPointInfo is a pointer to a                */
   /* GAVD_Local_End_Point_Info_t structure that contains all the       */
   /* necessary information needed to describe this local end point     */
   /* being registered.  The final two parameters, specify the GAVD     */
   /* Event Callback and Callback Parameter (respectively) that is to be*/
   /* registered to receive GAVD Events pertaining to the registered    */
   /* Endpoint.  This function returns a positive, non-zero value if    */
   /* successful.  This number is the Local Stream End Point ID (LSEID) */
   /* that must be used to identify this Stream End Point in future     */
   /* calls.  A negative error code will be returned if this routine    */
   /* fails.                                                            */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Register_End_Point(unsigned int BluetoothStackID, GAVD_Local_End_Point_Info_t *LocalEndPointInfo, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Register_End_Point_t)(unsigned int BluetoothStackID, GAVD_Local_End_Point_Info_t *LocalEndPointInfo, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* un-register previously registered Local Stream End Points from the*/
   /* local Stream End Point Manager.  This function accepts as input   */
   /* the Bluetooth Stack ID of the Bluetooth Protocol Stack that the   */
   /* Stream End Point Manager is valid for and the Local Stream End    */
   /* Point ID (LSEID) of the Local Stream End Point to un-register.    */
   /* This function returns zero on success or a negative error code on */
   /* failure.                                                          */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Un_Register_End_Point(unsigned int BluetoothStackID, unsigned int LSEID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Un_Register_End_Point_t)(unsigned int BluetoothStackID, unsigned int LSEID);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* establish a connection to a Remote End Point on a Remote Stream   */
   /* End Point Manager.  This function accepts as input, the Bluetooth */
   /* Stack ID of the Bluetooth Protocol Stack.  The second parameter,  */
   /* BD_ADDR, is the Bluetooth address of the remote device.  The third*/
   /* parameter, RSEID, is the ID of the Remote Stream End Point that   */
   /* the application wishes to open.  The LocalEndPointInfo is a       */
   /* pointer to a GAVD_Local_End_Point_Info_t structure that contains  */
   /* the necessary information for the local end point that will       */
   /* connect to the remote end point that is being opened (capabilities*/
   /* of the local end point).  The next two parameters,                */
   /* NumberConfigurationCapabilities and ConfigurationCapabilities,    */
   /* specify the number of configuration options and the configuration */
   /* options (respectively) that will be used to configure the remote  */
   /* endpoint.  The final two parameters, specify the GAVD Event       */
   /* Callback and Callback Parameter (respectively) that is to be      */
   /* registered to receive AVDTP Events pertaining to the established  */
   /* Endpoint.  This function returns a positive, non-zero value if    */
   /* successful.  This value is the Local Stream End Point ID (LSEID)  */
   /* that must be used to identify the Local Stream End Point and,     */
   /* consequently, the connection (stream) to the Remote Stream End    */
   /* Point in all other end point related routines.  This function     */
   /* returns a negative error code on failure.                         */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Open_Remote_End_Point(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, unsigned int RSEID, GAVD_Local_End_Point_Info_t *LocalEndPointInfo, unsigned int NumberConfigurationCapabilities, GAVD_Service_Capabilities_Info_t ConfigurationCapabilities[], GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Open_Remote_End_Point_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, unsigned int RSEID, GAVD_Local_End_Point_Info_t *LocalEndPointInfo, unsigned int NumberConfigurationCapabilities, GAVD_Service_Capabilities_Info_t ConfigurationCapabilities[], GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism to close a*/
   /* connection to an End Point that was opened previously via a call  */
   /* to GAVD_Register_End_Point() or GAVD_Open_Remote_End_Point().     */
   /* This function accepts as input, the Bluetooth Stack ID of the     */
   /* Bluetooth Protocol Stack and the LSEID is of the Local Stream End */
   /* Point.  This function returns zero on success or a negative error */
   /* code on failure.                                                  */
   /* * NOTE * Calling this function on a local End Point that was      */
   /*          registered via the GAVD_Register_End_Point() function    */
   /*          *DOES NOT* remove the End Point from the system.  This   */
   /*          function simply terminates any currently connected       */
   /*          End Point GAVD/AVDTP Client.                             */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Close_End_Point(unsigned int BluetoothStackID, unsigned int LSEID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Close_End_Point_t)(unsigned int BluetoothStackID, unsigned int LSEID);
#endif

   /* The following function is provided to allow a mechanism to respond*/
   /* to a request from a remote GAVD/AVDTP entity to Set the           */
   /* Configuration of a Stream End Point.  This function accepts as    */
   /* input, the Bluetooth Stack ID of the Bluetooth Protocol Stack, the*/
   /* LSEID of the Local Stream End Point that is being configured, the */
   /* First Failing Service Category (pass scNone if all succeed), and  */
   /* an Error Code identifying the reason for the failure (0 if no     */
   /* errors).  This function returns zero if successful, or a negative */
   /* error code if there was an error.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Set_Configuration_Response(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Service_Category_t FirstFailingServiceCategory, unsigned int ErrorCode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Set_Configuration_Response_t)(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Service_Category_t FirstFailingServiceCategory, unsigned int ErrorCode);
#endif

   /* The following function is provided to allow a mechanism to request*/
   /* to start one or more streams on the remote GAVD/AVDTP device.     */
   /* This function accepts as input, the Bluetooth Stack ID of the     */
   /* Bluetooth Protocol Stack, the number of streams to start, and a   */
   /* list of Local Stream End Point IDs (LSEIDs) for these streams.    */
   /* This function returns zero if successful, or a negative return    */
   /* value if there was an error.                                      */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Start_Stream_Request(unsigned int BluetoothStackID, unsigned int NumberStreams, unsigned int LSEID[]);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Start_Stream_Request_t)(unsigned int BluetoothStackID, unsigned int NumberStreams, unsigned int LSEID[]);
#endif

   /* The following function is provided to allow a mechanism to respond*/
   /* to a request from a remote GAVD/AVDTP entity to start one or more */
   /* streams.  This function accepts as input, the Bluetooth Stack ID  */
   /* of the Bluetooth Protocol Stack, the LSEID of the Stream End Point*/
   /* that this response is for, and an error code identifying the      */
   /* reason for the failure (0 if no errors).  This function returns   */
   /* zero if successful, or a negative return value if there was an    */
   /* error.                                                            */
   /* * NOTE * This function CAN ONLY BE CALLED from within the context */
   /*          of the Stream Endpoint Event Callback function in        */
   /*          response to an Endpoint Start Request !!!!!!!!!!!!!!!!!! */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Start_Stream_Response(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Start_Stream_Response_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode);
#endif

   /* The following function is provided to allow a mechanism to request*/
   /* the suspension of one or more streams on the remote GAVD/AVDTP    */
   /* device.  This function accepts as input, the Bluetooth Stack ID of*/
   /* the Bluetooth Protocol Stack, and a list of Local Stream End Point*/
   /* IDs (LSEIDs) for these streams.  This function returns zero if    */
   /* successful, or a negative return value if there was an error.     */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Suspend_Stream_Request(unsigned int BluetoothStackID, unsigned int NumberStreams, unsigned int LSEID[]);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Suspend_Stream_Request_t)(unsigned int BluetoothStackID, unsigned int NumberStreams, unsigned int LSEID[]);
#endif

   /* The following function is provided to allow a mechanism to respond*/
   /* to a request from a remote GAVD/AVDTP entity to suspend one or    */
   /* more streams.  This function accepts as input, the Bluetooth Stack*/
   /* ID of the Bluetooth Protocol Stack, the LSEID of the Local Stream */
   /* End Point responding to the Suspend Request, and an error code    */
   /* identifying the reason for the failure (0 if no errors).  This    */
   /* function returns zero if successful, or a negative return value if*/
   /* there was an error.                                               */
   /* * NOTE * This function CAN ONLY BE CALLED from within the context */
   /*          of the Stream Endpoint Event Callback function in        */
   /*          response to an Endpoint Suspend Request !!!!!!!!!!!!!!!! */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Suspend_Stream_Response(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Suspend_Stream_Response_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode);
#endif

   /* The following function is provided to allow a mechanism to request*/
   /* the reconfiguration of an Endpoint on the remote device.  This    */
   /* function accepts as input, the Bluetooth Stack ID of the Bluetooth*/
   /* Protocol Stack, the LSEID of the Local Stream End Point that is   */
   /* connected to the stream being reconfigured, the number of Service */
   /* Capabilities to reconfigure, and a pointer to an array of Service */
   /* Capabilities to Reconfigure.  This function returns zero if       */
   /* successful, or a negative return value if there was an error.     */
   /* * NOTE * The ONLY valid Service Capabilities for this routine are:*/
   /*             (1) Media Codec Capabilities.                         */
   /*             (2) Content Protection Capabilities.                  */
   /*          The above Service Capabilities are the only Capabilities */
   /*          allowed to be reconfigured by AVDTP.                     */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Reconfigure_Request(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberServiceCapabilities, GAVD_Service_Capabilities_Info_t ServiceCapabilities[]);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Reconfigure_Request_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberServiceCapabilities, GAVD_Service_Capabilities_Info_t ServiceCapabilities[]);
#endif

   /* The following function is provided to allow a mechanism to respond*/
   /* to a request from a remote GAVD/AVDTP entity to reconfigure a     */
   /* stream.  This function accepts as input, the Bluetooth Stack ID of*/
   /* the Bluetooth Protocol Stack, the LSEID of the Local Stream End   */
   /* Point that is connected to the stream being reconfigured, the     */
   /* First Failing Service Category (pass scNone if all succeed), and  */
   /* an Error Code identifying the reason for the failure (0 if no     */
   /* errors).  This function returns zero if successful, or a negative */
   /* error code if there was an error.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Reconfigure_Response(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Service_Category_t FirstFailingServiceCategory, unsigned int ErrorCode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Reconfigure_Response_t)(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Service_Category_t FirstFailingServiceCategory, unsigned int ErrorCode);
#endif

   /* The following function is provided to allow a mechanism to request*/
   /* the change of the security settings for a stream.  This function  */
   /* accepts as input, the Bluetooth Stack ID of the Bluetooth Protocol*/
   /* Stack, the LSEID of the Local Stream End Point that is connected  */
   /* to the stream, the length of the security (Content Protection)    */
   /* Data, and a pointer to the Content Protection Data.  This function*/
   /* returns zero if successful, or a negative return value if there   */
   /* was an error.                                                     */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Security_Control_Request(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ContentProtectionDataLength, Byte_t *ContentProtectionData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Security_Control_Request_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ContentProtectionDataLength, Byte_t *ContentProtectionData);
#endif

   /* The following function is provided to allow the application to    */
   /* respond to a request from a remote GAVD/AVDTP entity to change the*/
   /* security settings for a stream.  This function accepts as input,  */
   /* the Bluetooth Stack ID of the Bluetooth Protocol Stack, the LSEID */
   /* of the Local Stream End Point that is connected to the stream, an */
   /* Error Code (0 if none), the length of the security (Content       */
   /* Protection) Data, and a pointer to the Content Protection Data.   */
   /* This function returns zero if successful, or a negative error code*/
   /* if there was an error.                                            */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Security_Control_Response(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode, unsigned int ContentProtectionDataLength, Byte_t *ContentProtectionData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Security_Control_Response_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int ErrorCode, unsigned int ContentProtectionDataLength, Byte_t *ContentProtectionData);
#endif

   /* The following function is provided to allow a mechanism to request*/
   /* to abort one or more streams on the remote GAVD/AVDTP device.     */
   /* This function accepts as input, the Bluetooth Stack ID of the     */
   /* Bluetooth Protocol Stack, and the ID of the Local Stream End Point*/
   /* (LSEID) that is connected to the stream.  This function returns   */
   /* zero if successful, or a negative return value if there was an    */
   /* error.                                                            */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Abort_Stream_Request(unsigned int BluetoothStackID, unsigned int LSEID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Abort_Stream_Request_t)(unsigned int BluetoothStackID, unsigned int LSEID);
#endif

   /* The following function is responsible for Sending Data over the   */
   /* the specified GAVD/AVDTP Stream.  This function accepts as input, */
   /* the Bluetooth Stack ID of the Bluetooth Protocol Stack.  The next */
   /* parameter is the LSEID of the Local Stream End Point sourcing the */
   /* data.  The Marker parameter specifies a marker bit used by certain*/
   /* Codec payload formats.  The PayloadType parameter indicates the   */
   /* content type of the data.  The TimeStamp parameter holds Timestamp*/
   /* information.  The DataLength parameter specifies the length of the*/
   /* payload data.  Finally, the DataBuffer parameter is a pointer to  */
   /* the payload data.  This function returns zero on a successful call*/
   /* or a negative return error code if the function call was not      */
   /* successful.                                                       */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Data_Write(unsigned int BluetoothStackID, unsigned int LSEID, Boolean_t Marker, Byte_t PayloadType, DWord_t TimeStamp, Word_t DataLength, Byte_t *DataBuffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Data_Write_t)(unsigned int BluetoothStackID, unsigned int LSEID, Boolean_t Marker, Byte_t PayloadType, DWord_t TimeStamp, Word_t DataLength, Byte_t *DataBuffer);
#endif

   /* The following function is responsible for sending Sender Report   */
   /* Data over the specified GAVD/AVDTP Stream.  This function accepts */
   /* as input, the Bluetooth Stack ID of the Bluetooth Protocol Stack. */
   /* The next parameter is the LSEID of the Local Stream End Point     */
   /* sourcing the data.  The SenderInfo parameter points to the Sender */
   /* Information in the Sender Report.  The NumberReportBlocks         */
   /* parameters tells how many report blocks are contained in an array */
   /* of report blocks pointed to by ReportBlocks parameter.  The       */
   /* ExtensionDataLength parameter tells how many 32-bit words are in  */
   /* the extension data pointed to by ExtensionData parameter.  This   */
   /* function returns zero on a successful call or a negative return   */
   /* error code if the function call was not successful.               */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Sender_Report_Data_Write(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Sender_Info_t *SenderInfo, unsigned int NumberReportBlocks, GAVD_Report_Block_t *ReportBlocks, Word_t ExtensionDataLength, DWord_t *ExtensionData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Sender_Report_Data_Write_t)(unsigned int BluetoothStackID, unsigned int LSEID, GAVD_Sender_Info_t *SenderInfo, unsigned int NumberReportBlocks, GAVD_Report_Block_t *ReportBlocks, Word_t ExtensionDataLength, DWord_t *ExtensionData);
#endif

   /* The following function is responsible for sending Receiver Report */
   /* Data over the specified GAVD/AVDTP Stream.  This function accepts */
   /* as input, the Bluetooth Stack ID of the Bluetooth Protocol Stack. */
   /* The next parameter is the LSEID of the Local Stream End Point     */
   /* sourcing the data.  The NumberReportBlocks parameter tells how    */
   /* many report blocks are contained in an array of report blocks     */
   /* pointed to by ReportBlocks parameter.  The ExtensionDataLength    */
   /* parameter tells how many 32-bit words are in the extension data   */
   /* pointed to by ExtensionData parameter.  This function returns zero*/
   /* on a successful call or a negative return error code if the       */
   /* function call was not successful.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Receiver_Report_Data_Write(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberReportBlocks, GAVD_Report_Block_t *ReportBlocks, Word_t ExtensionDataLength, DWord_t *ExtensionData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Receiver_Report_Data_Write_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberReportBlocks, GAVD_Report_Block_t *ReportBlocks, Word_t ExtensionDataLength, DWord_t *ExtensionData);
#endif

   /* The following function is responsible for sending SDES Report Data*/
   /* over the specified GAVD/AVDTP Stream.  This function accepts as   */
   /* input, the Bluetooth Stack ID of the Bluetooth Protocol Stack.    */
   /* The next parameter is the LSEID of the Local Stream End Point     */
   /* sourcing the data.  The NumberSDESChunks parameter tells how many */
   /* SDES Chunks are contained in the array of SDES Chunks pointed to  */
   /* by SDESChunks parameter (final parameter).  This function returns */
   /* zero on a successful call or a negative return error code if the  */
   /* function call was not successful.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_SDES_Report_Data_Write(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberSDESChunks, GAVD_SDES_Chunk_t *SDESChunks);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_SDES_Report_Data_Write_t)(unsigned int BluetoothStackID, unsigned int LSEID, unsigned int NumberSDESChunks, GAVD_SDES_Chunk_t *SDESChunks);
#endif

   /* The following function is responsible for sending RTP FEC Recovery*/
   /* Data over the specified GAVD/AVDTP Stream.  This function accepts */
   /* as input, the Bluetooth Stack ID of the Bluetooth Protocol Stack. */
   /* The next parameter is the LSEID of the Local Stream End Point     */
   /* sourcing the data.  The Marker parameter specifies a marker bit   */
   /* used by certain Codec payload formats.  The PayloadType parameter */
   /* indicates the content type of the data which should be the same   */
   /* value as the corresponding media stream's payload type.  The      */
   /* TimeStamp parameter holds Timestamp information.  The FECBlock    */
   /* parameter specifies a pointer to the GAVD_FEC_Block_t structure   */
   /* containing the information to send..  This function returns zero  */
   /* on a successful call or a negative return error code if the       */
   /* function call was not successful.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Recovery_Data_Write(unsigned int BluetoothStackID, unsigned int LSEID, Boolean_t Marker, Byte_t PayloadType, DWord_t TimeStamp, GAVD_FEC_Block_t *FECBlock);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Recovery_Data_Write_t)(unsigned int BluetoothStackID, unsigned int LSEID, Boolean_t Marker, Byte_t PayloadType, DWord_t TimeStamp, GAVD_FEC_Block_t *FECBlock);
#endif

   /* The following function is responsible for retrieving the current  */
   /* GAVD/AVDTP Server Connection Mode.  This function accepts as its  */
   /* first parameter the Bluetooth Stack ID of the Bluetooth Stack on  */
   /* which the server exists.  The final parameter to this function is */
   /* a pointer to a Server Connection Mode variable which will receive */
   /* the current Server Connection Mode.  This function returns zero if*/
   /* successful, or a negative return error code if an error occurred. */
   /* ** NOTE ** The Default Server Connection Mode is                  */
   /*            gsmAutomaticAccept.                                    */
   /* ** NOTE ** This function is used for GAVD/AVDTP Servers which use */
   /*            Bluetooth Security Mode 2.                             */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Get_Server_Connection_Mode(unsigned int BluetoothStackID, GAVD_Server_Connection_Mode_t *ServerConnectionMode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Get_Server_Connection_Mode_t)(unsigned int BluetoothStackID, GAVD_Server_Connection_Mode_t *ServerConnectionMode);
#endif

   /* The following function is responsible for setting the GAVD/AVDTP  */
   /* Server Connection Mode.  This function accepts as its first       */
   /* parameter the Bluetooth Stack ID of the Bluetooth Stack in which  */
   /* the server exists.  The second parameter to this function is the  */
   /* new Server Connection Mode to set the Server to use.  The final   */
   /* two parameters to this function specify an GAVD/AVDTP Event       */
   /* Callback (and callback parameter, respectively) which are used to */
   /* receive notifications of a Bluetooth Connection Request.  These   */
   /* connection requests will not be dispatched unless the Server Mode */
   /* (second parameter) is set to gsmManualAccept.  In this case the   */
   /* Callback will be invoked whenever a Remote Bluetooth Device       */
   /* attempts to connect to the Local Device.  If the Server Mode      */
   /* (second parameter) is se to anything other than gsmManualAccept   */
   /* then the final two parameters are ignored.  Note that only        */
   /* Connection Request events will be dispatched to the specified     */
   /* callback.  No other events will be dispatched.  This function     */
   /* returns zero if successful, or a negative return error code if an */
   /* error occurred.                                                   */
   /* ** NOTE ** The Default Server Connection Mode is                  */
   /*            gsmAutomaticAccept.                                    */
   /* ** NOTE ** This function is used for GAVD/AVDTP Servers which use */
   /*            Bluetooth Security Mode 2.                             */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Set_Server_Connection_Mode(unsigned int BluetoothStackID, GAVD_Server_Connection_Mode_t ServerConnectionMode, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Set_Server_Connection_Mode_t)(unsigned int BluetoothStackID, GAVD_Server_Connection_Mode_t ServerConnectionMode, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is a utility function that exists to allow */
   /* the user a mechanism to register to receive Connection Status     */
   /* Events.  These events simply allow the ability to monitor GAVD    */
   /* Signalling Channel connections and disconnections.  This function */
   /* accepts as input, the Bluetooth Stack ID of the Bluteooth Stack   */
   /* that is monitor connection status, followed by the Connection     */
   /* Status Event Callback information (actual callback function and   */
   /* parameter).  This function returns a positive, non-zero value if  */
   /* successful, or a negative return error code if there was an error.*/
   /* * NOTE * If this function is successful, the                      */
   /*          GAVD_Register_Signalling_Connection_Status() function can*/
   /*          be called (passing the return value from this function)  */
   /*          to un-register the registered callback.                  */
   /* * NOTE * The registered callback will only receive event          */
   /*          callbacks with the specified event types:                */
   /*             - etGAVD_Signalling_Connect_Indication                */
   /*             - etGAVD_Signalling_Disconnect_Indication             */
   /* * NOTE * There can only be a single Connection Status Event       */
   /*          Callback registered at any given time.                   */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Register_Signalling_Connection_Status(unsigned int BluetoothStackID, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Register_Signalling_Connection_Status_t)(unsigned int BluetoothStackID, GAVD_Event_Callback_t GAVDEventCallback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* Un-Register a previously registered GAVD Event Connection Status  */
   /* Registration (via a successful call to the                        */
   /* GAVD_Register_Signalling_Connection_Status() function.  This      */
   /* function accepts as input the Bluetooth Stack ID followed by the  */
   /* GAVD Event Connection Status Callback ID (return value from the   */
   /* GAVD_Register_Signalling_Connection_Status() function).  This     */
   /* function returns zero if successful or a negative return error    */
   /* code if there was an error.                                       */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Un_Register_Signalling_Connection_Status(unsigned int BluetoothStackID, unsigned int GAVDConnectionStatusID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Un_Register_Signalling_Connection_Status_t)(unsigned int BluetoothStackID, unsigned int GAVDConnectionStatusID);
#endif

   /* The following function is a utility function that exists to allow */
   /* the user a mechanism of disconnecting ANY currently connected     */
   /* remote Bluetooth device.  This is accomplished by disconnecting   */
   /* any Signalling Channel that is connected for the specified Remote */
   /* Bluetooth Device (specified by device address).  This function    */
   /* accepts as input the Bluetooth Stack ID followed by the Remote    */
   /* Bluetooth Device to disconnect.  This function returns zero if    */
   /* successful, of a negative return error code if there was an error.*/
BTPSAPI_DECLARATION int BTPSAPI GAVD_Disconnect_Signalling_Connection(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Disconnect_Signalling_Connection_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is a utility function that exists to query */
   /* the currently configured GAVD Data Queing parameters.  These      */
   /* parameters dictate how the data packets are queued into L2CAP.    */
   /* This mechanism allows for the ability to implement a streaming    */
   /* type interface by limiting the number of packets that can queued. */
   /* This is useful to keep L2CAP from infinitely queing packets which */
   /* can lead to stale data if there is an issue sending the data to   */
   /* the remote device (i.e. interference).  This function accepts the */
   /* Bluetooth Stack ID followed by a pointer to a buffer that is to   */
   /* receive the current L2CAP Queueing parameters that are being used */
   /* by GAVD.  This function returns zero if successful or a negative  */
   /* return error code if there was an error.                          */
   /* * NOTE * This function sets the queing parameters globally for    */
   /*          GAVD.  Setting the Queing parameters for an individual   */
   /*          stream endpoint is currently not supported.  This is     */
   /*          because multiple stream endpoints could be multiplexed   */
   /*          over the same channel.                                   */
   /* * NOTE * A value of zero for the QueueLimit member of the L2CAP   */
   /*          queing parameters means that there is no queing active   */
   /*          (i.e. all packets are queued, regardless of the queue    */
   /*          depth).                                                  */
   /* * NOTE * It is recommended to ALWAYS use the                      */
   /*             L2CA_QUEUEING_FLAG_DISCARD_OLDEST flag when specifying*/
   /*          queing parameters.  Although the threshold method can    */
   /*          be used (by having the queue fail and waiting for a      */
   /*          Data buffer empty indication), it is recommended to      */
   /*          allow the data to always be queued and have the oldest   */
   /*          data deleted from the queue and have the newest data     */
   /*          placed at the end of the queue (i.e. always queue the    */
   /*          data).                                                   */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Get_Data_Queueing_Parameters(unsigned int BluetoothStackID, L2CA_Queueing_Parameters_t *QueueingParameters);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Get_Data_Queueing_Parameters_t)(unsigned int BluetoothStackID, L2CA_Queueing_Parameters_t *QueueingParameters);
#endif

   /* The following function is a utility function that exists to change*/
   /* the currently configured GAVD Data Queing parameters.  These      */
   /* parameters dictate how the data packets are queued into L2CAP.    */
   /* This mechanism allows for the ability to implement a streaming    */
   /* type interface by limiting the number of packets that can queued. */
   /* This is useful to keep L2CAP from infinitely queing packets which */
   /* can lead to stale data if there is an issue sending the data to   */
   /* the remote device (i.e. interference).  This function accepts the */
   /* Bluetooth Stack ID followed by a pointer to a buffer that contains*/
   /* the new L2CAP Queueing parameters that are to be used by GAVD.    */
   /* This function returns zero if successful or a negative return     */
   /* error code if there was an error.                                 */
   /* * NOTE * This function sets the queing parameters globally for    */
   /*          GAVD.  Setting the Queing parameters for an individual   */
   /*          stream endpoint is currently not supported.  This is     */
   /*          because multiple stream endpoints could be multiplexed   */
   /*          over the same channel.                                   */
   /* * NOTE * A value of zero for the QueueLimit member of the L2CAP   */
   /*          queing parameters means that there is no queing active   */
   /*          (i.e. all packets are queued, regardless of the queue    */
   /*          depth).                                                  */
   /* * NOTE * It is recommended to ALWAYS use the                      */
   /*             L2CA_QUEUEING_FLAG_DISCARD_OLDEST flag when specifying*/
   /*          queing parameters.  Although the threshold method can    */
   /*          be used (by having the queue fail and waiting for a      */
   /*          Data buffer empty indication), it is recommended to      */
   /*          allow the data to always be queued and have the oldest   */
   /*          data deleted from the queue and have the newest data     */
   /*          placed at the end of the queue (i.e. always queue the    */
   /*          data).                                                   */
BTPSAPI_DECLARATION int BTPSAPI GAVD_Set_Data_Queueing_Parameters(unsigned int BluetoothStackID, L2CA_Queueing_Parameters_t *QueueingParameters);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAVD_Set_Data_Queueing_Parameters_t)(unsigned int BluetoothStackID, L2CA_Queueing_Parameters_t *QueueingParameters);
#endif

#endif
