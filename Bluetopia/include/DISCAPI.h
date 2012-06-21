/*****< DISCAPI.h >************************************************************/
/*      Copyright 2005-2011 Stonestreet One, Inc.                             */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  DISCAPI - Service Dicovery Module Type Definitions, Prototypes, and       */
/*            Constants.                                                      */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   03/10/10  T. Thomas      Initial creation.                               */
/******************************************************************************/
#ifndef __DISCAPIH__
#define __DISCAPIH__

#include "SS1BTPS.h"            /* Bluetooth Stack API Prototypes/Constants.  */

   /* Error Return Codes.                                               */

   /* Error Codes that are smaller than these (less than -1000) are     */
   /* related to the Bluetooth Protocol Stack itself (see BTERRORS.H).  */
#define DISC_ERROR_INVALID_PARAMETER                           (-1000)
#define DISC_ERROR_NOT_INITIALIZED                             (-1001)
#define DISC_ERROR_INVALID_BLUETOOTH_STACK_ID                  (-1002)
#define DISC_ERROR_INSUFFICIENT_RESOURCES                      (-1003)
#define DISC_ERROR_INTERNAL_ERROR                              (-1004)
#define DISC_ERROR_ACTION_NOT_ALLOWED                          (-1005)
#define DISC_ERROR_DEVICE_LIST_EMPTY                           (-1006)
#define DISC_ERROR_PROFILE_LIST_EMPTY                          (-1007)
#define DISC_ERROR_INVALID_PROFILE_ID                          (-1008)

   /* The following defines the enumerated types that describe the      */
   /* profile to which the profile information belongs.                 */
typedef enum
{
  piSPP,
  piHID,
  piHDS,
  piHDS_AG,
  piHFRE,
  piHFRE_AG,
  piUNKNOWN
} Profile_Identifier_t;

   /* The following defines the SDP Information that pertains to the SPP*/
   /* Profile.                                                          */
typedef struct _tagSPP_Info_t
{
   Byte_t ServerChannel;
} SPP_Info_t;

   /* The following defines the mask values that are used to isolate    */
   /* information about the supported features for the HID Profile.     */
#define HID_PROFILE_FLAG_VIRTUAL_CABLE_SUPPORT                 0x0001
#define HID_PROFILE_FLAG_RECONNECT_INITIATE_SUPPORT            0x0002
#define HID_PROFILE_FLAG_HID_SDP_DISABLE                       0x0004
#define HID_PROFILE_FLAG_BATTERY_POWERED                       0x0008
#define HID_PROFILE_FLAG_REMOTE_WAKE_SUPPORT                   0x0010
#define HID_PROFILE_FLAG_BOOT_DEVICE                           0x0020
#define HID_PROFILE_FLAG_NORMALLY_CONNECTABLE                  0x0040

   /* The following defines the SDP Information that pertains to the HID*/
   /* Profile.                                                          */
typedef struct _tagHID_Info_t
{
   Word_t  ProfileVersion;
   Word_t  HIDVersion;
   Word_t  ControlPSM;
   Word_t  InterruptPSM;
   Word_t  SupervisionTimeout;
   Word_t  Flags;
   Byte_t  ReportDescriptorLength;
   Byte_t *ReportDescriptor;
} HID_Info_t;

   /* The following defines the SDP Information that pertains to the    */
   /* Headset Profile.                                                  */
typedef struct _tagHDS_Info_t
{
   Byte_t    ServerChannel;
   Word_t    ProfileVersion;
   Boolean_t RemoteAudioVolumnControl;
} HDS_Info_t;

   /* The following defines the mask values that are used to isolate    */
   /* information about the supported features for the Hands Free       */
   /* Profile.                                                          */
#define HFRE_PROFILE_FLAG_EC_AND_OR_NR_SUPPORT                 0x0001
#define HFRE_PROFILE_FLAG_CALL_WAITING_AND_3_WAY_CALL_SUPPORT  0x0002
#define HFRE_PROFILE_FLAG_CLI_PRESENTATION_SUPPORT             0x0004
#define HFRE_PROFILE_FLAG_VOICE_RECOGNITION_SUPPORT            0x0008
#define HFRE_PROFILE_FLAG_REMOTE_AUDIO_VOLUME_CONTROL          0x0010

#define HFRE_PROFILE_FLAG_3_WAY_CALL_SUPPORT                   0x0020
#define HFRE_PROFILE_FLAG_IN_BAND_RING_TONE_SUPPORT            0x0040
#define HFRE_PROFILE_FLAG_ATTACH_NUMBER_TO_VOICE_TAG_SUPPORT   0x0080
#define HFRE_PROFILE_FLAG_CALL_REJECT_SUPPORT                  0x0100

   /* The following defines the SDP Information that pertains to the    */
   /* Hands Free Profile.                                               */
typedef struct _tagHFRE_Info_t
{
   Byte_t ServerChannel;
   Word_t ProfileVersion;
   Word_t Flags;
} HFRE_Info_t;

   /* The following defines the structure that contains information     */
   /* about a specific profile supported on the remote device.  The     */
   /* Profile Identifier value identifies the Profile Information       */
   /* structure that is used to access the information about the        */
   /* profile.                                                          */
typedef struct _tagProfile_Info_t
{
   Word_t                ServiceNameLength;
   Byte_t               *ServiceName;
   Word_t                ServiceDescLength;
   Byte_t               *ServiceDesc;
   Word_t                ServiceProviderLength;
   Byte_t               *ServiceProvider;
   Profile_Identifier_t  ProfileIdentifier;
   union
   {
      SPP_Info_t  SPPInfo;
      HID_Info_t  HIDInfo;
      HDS_Info_t  HDSInfo;
      HFRE_Info_t HFREInfo;
   } Profile;
} Profile_Info_t;

#define PROFILE_INFO_DATA_SIZE                  (sizeof(Profile_Info_t))

   /* The following defines the structure that is used to contain all of*/
   /* the information about the device that has been discovered.        */
typedef struct _tagDevice_Info_t
{
   BD_ADDR_t          BD_ADDR;
   Class_of_Device_t  ClassOfDevice;
   Word_t             ClockOffset;
   Byte_t             Page_Scan_Repetition_Mode;
   Boolean_t          NameValid;
   char              *DeviceName;
} Device_Info_t;

#define DEVICE_INFO_DATA_SIZE                               (sizeof(Device_Info_t))

   /* The following defines the optional filter structure that may be   */
   /* passed into DISC_Device_Discovery_Start that may be used to       */
   /* filter the devices returned from a device discovery procedure.    */
   /* * NOTE * If ClassOfDeviceMask OR LAP are set to all 0s then that  */
   /*          filter will not be used.                                 */
   /* * NOTE * If both members are set to Non-Zero then both filters    */
   /*          will be applied to the discovery procedure.              */
typedef struct _tagDevice_Filter_t
{
   Class_of_Device_t ClassOfDeviceMask;
   LAP_t             LAP;
} Device_Filter_t;

#define DEVICE_FILTER_DATA_SIZE                             (sizeof(Device_Filter_t))

   /* The following defines the structure that is used to contain all of*/
   /* the information about the device that has been discovered.  The   */
   /* parameter ProfileInfo is a pointer to an array of Profile_Info_t  */
   /* structures.  The number of array elements is defined by the       */
   /* NumberOfProfiles parameter.                                       */
typedef struct _tagService_Info_t
{
   BD_ADDR_t       BD_ADDR;
   int             NumberOfProfiles;
   Profile_Info_t *ProfileInfo;
} Service_Info_t;

#define SERVICE_INFO_DATA_SIZE                              (sizeof(Service_Info_t))

   /* DISC Event API Types.                                             */
typedef enum
{
   etDISC_Device_Information_Indication,
   etDISC_Service_Information_Indication,
   etDISC_Service_Search_Error_Indication,
} DISC_Event_Type_t;

   /* The following DISC Device Information Indication Event is         */
   /* dispatched when DISC has located a new device and the information */
   /* about the new device is determined.                               */
typedef struct _tagDISC_Device_Information_Indication_Data_t
{
   Device_Info_t DeviceInfo;
} DISC_Device_Information_Indication_Data_t;

#define DISC_DEVICE_INFORMATION_INDICATION_DATA_SIZE        (sizeof(DISC_Device_Information_Indication_Data_t))

   /* The following DISC Service Information Indication Event is        */
   /* dispatched when DISC has located Services on a remote device that */
   /* was being searched for.  The Raw_SDP_Response_Data is a pointer to*/
   /* the Raw SDP information that was searched to provide the Service  */
   /* Information.  This is provided to allow a user to extend the      */
   /* functionality by manually parsing the data for information that is*/
   /* not currently supported in this version of the module.            */
typedef struct _tagDISC_Service_Information_Indication_Data_t
{
   Service_Info_t       ServiceInfo;
   SDP_Response_Data_t *Raw_SDP_Response_Data;
} DISC_Service_Information_Indication_Data_t;

#define DISC_SERVICE_INFORMATION_INDICATION_DATA_SIZE       (sizeof(DISC_Service_Information_Indication_Data_t))

   /* DISC Error Event, Error Type API Types.                           */
typedef enum
{
   etRequestFailure,
   etRequestTimeout,
   etConnectionError,
   etErrorResponse,
   etMemoryAllocationFailure,
   etUnknownError
} DISC_SDP_Error_Type_t;

   /* The following DISC Service Search Error Indication Event is       */
   /* dispatched when DISC has encountered an error with an SDP Service */
   /* Search request operation.  BD_ADDR contains the BD_ADDR of the    */
   /* device that was being processed during a etMemoryAllocationFailure*/
   /* occurred.  ResultCode contains the error return value from SDP on */
   /* a etRequestFailure.  SDP_Error_Response_Data contains information */
   /* returned from SDP when an etErrorResponse is received from SDP.   */
   /* All other error event types will contain no additional            */
   /* information.                                                      */
typedef struct _tagDISC_Service_Search_Error_Indication_Data_t
{
   BD_ADDR_t                  BD_ADDR;
   DISC_SDP_Error_Type_t      Error_Type;
   SDP_Error_Response_Data_t *SDP_Error_Response_Data;
} DISC_Service_Search_Error_Indication_Data_t;

#define DISC_SERVICE_SERARCH_ERROR_INDICATION_DATA_SIZE     (sizeof(DISC_Service_Search_Error_Indication_Data_t))

   /* The following structure represents the container structure for    */
   /* Holding all DISC Event Data Data.                                 */
typedef struct _tagDISC_Event_Data_t
{
   DISC_Event_Type_t Event_Data_Type;
   Word_t            Event_Data_Size;
   union
   {
      DISC_Device_Information_Indication_Data_t   *DISC_Device_Information_Indication_Data;
      DISC_Service_Information_Indication_Data_t  *DISC_Service_Information_Indication_Data;
      DISC_Service_Search_Error_Indication_Data_t *DISC_Service_Search_Error_Indication_Data;
   } Event_Data;
} DISC_Event_Data_t;

#define DISC_EVENT_DATA_SIZE                                (sizeof(DISC_Event_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* a DISC Profile Event Data Callback.  This function will be called */
   /* whenever a DISC Event occurs that is associated with the specified*/
   /* Bluetooth Stack ID.  This function passes to the caller the       */
   /* Bluetooth Stack ID, the DISC Event Data that occurred and the DISC*/
   /* Event Callback Parameter that was specified when this Callback was*/
   /* installed.  The caller is free to use the contents of the DISC    */
   /* Event Data ONLY in the context of this callback.  If the caller   */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e.  this   */
   /* function DOES NOT have be reentrant).  It needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another DISC Profile Event    */
   /* will not be processed while this function call is outstanding).   */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving DISC Event Packets. */
   /*            A Deadlock WILL occur because NO DISC Event Callbacks  */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
typedef void (BTPSAPI *DISC_Event_Callback_t)(unsigned int BluetoothStackID, DISC_Event_Data_t *DISC_Event_Data, unsigned long CallbackParameter);

   /* The following function is responsible for initializing an DISC    */
   /* Context Layer for the specified Bluetooth Protocol Stack.  This   */
   /* function will allocate and initialize an DISC Context Information */
   /* structure associated with the specified Bluetooth Stack ID.  This */
   /* function returns zero if successful, or a non-zero value if there */
   /* was an error.                                                     */
BTPSAPI_DECLARATION int BTPSAPI DISC_Initialize(unsigned int BluetoothStackID);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_DISC_Initialize_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is responsible for releasing any resources */
   /* that the DISC Layer associated with the Bluetooth Protocol Stack, */
   /* specified by the Bluetooth Stack ID, has allocated.  Upon         */
   /* completion of this function, ALL DISC functions will fail if used */
   /* on the specified Bluetooth Protocol Stack.                        */
BTPSAPI_DECLARATION void BTPSAPI DISC_Cleanup(void);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_DISC_Cleanup_t)(void);
#endif

   /* The following function is used to initiate the Device Discovery   */
   /* process.  The function takes as its first parameter the           */
   /* BluetoothStackID that is associated with the Bluetooth Device.    */
   /* The second parameter is an optional filter structure that may be  */
   /* used to filter the devices returned from the Device Discovery     */
   /* procedure. The next parameter is a pointer to the callback        */
   /* function that will receive the device information as it becomes   */
   /* available.  The final parameter is a user defined value that will */
   /* be returned to in the Callback Parameter of the callback function.*/
   /* The user is free to use this for any purpose.  The function       */
   /* returns a negative return value if there was an error and Zero in */
   /* success.                                                          */
BTPSAPI_DECLARATION int BTPSAPI DISC_Device_Discovery_Start(unsigned int BluetoothStackID, Device_Filter_t *DeviceFilter, DISC_Event_Callback_t DiscoveryCallback, unsigned long DiscoveryCallbackParameter);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_DISC_Device_Discovery_Start_t)(unsigned int BluetoothStackID, Device_Filter_t *DeviceFilter, DISC_Event_Callback_t DiscoveryCallback, unsigned long DiscoveryCallbackParameter);
#endif

   /* The following function is used to terminate the Device Discovery  */
   /* process.  The function takes as its parameter the BluetoothStackID*/
   /* that is associated with the Bluetooth Device.  The function       */
   /* returns a negative return value if there was an error and Zero in */
   /* success.                                                          */
BTPSAPI_DECLARATION int BTPSAPI DISC_Device_Discovery_Stop(unsigned int BluetoothStackID);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_DISC_Device_Discovery_Stop_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is used to initiate the Service Discovery  */
   /* process or queue additional requests.  The function takes as its  */
   /* first parameter the BluetoothStackID that is associated with the  */
   /* Bluetooth Device.  The second parameter is the BD_ADDR of the     */
   /* device that is to be searched.  The third and fourth parameters   */
   /* indicates and list the profiles that are to be searched for.  The */
   /* final two parameters define the Callback function and parameter to*/
   /* use when the service discovery is complete.  The function returns */
   /* zero on success and a negative return value if there was an error.*/
   /* * NOTE * This function may be called a number of times.  The first*/
   /*          call to this function will initiate the discovery        */
   /*          request.  If another call is made to this function while */
   /*          a current discovery process is active, the information   */
   /*          will be placed into a queue and processed in the order   */
   /*          that the requests were made when the current discovery   */
   /*          process completes.                                       */
BTPSAPI_DECLARATION int BTPSAPI DISC_Service_Discovery_Start(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, int NumberOfProfiles, Profile_Identifier_t *ProfileIDList, DISC_Event_Callback_t ServiceDiscoveryCallback, unsigned long ServiceDiscoveryCallbackParameter);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_DISC_Service_Discovery_Start_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, int NumberOfProfiles, Profile_Identifier_t *ProfileIDList, DISC_Event_Callback_t ServiceDiscoveryCallback, unsigned long ServiceDiscoveryCallbackParameter);
#endif

   /* The following function is used to terminate the Device Discovery  */
   /* process.  The function takes as its parameter the BluetoothStackID*/
   /* that is associated with the Bluetooth Device.  The function       */
   /* returns a negative return value if there was an error and Zero in */
   /* success.                                                          */
   /* * NOTE * This function will cancel any discovery operations that  */
   /*          are currently in progress and release all request        */
   /*          information in the queue that are waiting to be executed.*/
BTPSAPI_DECLARATION int BTPSAPI DISC_Service_Discovery_Stop(unsigned int BluetoothStackID);

#ifdef INCLUDE_DEBUG_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_DISC_Service_Discovery_Stop_t)(unsigned int BluetoothStackID);
#endif

#endif
