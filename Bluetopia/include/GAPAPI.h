/*****< gapapi.h >*************************************************************/
/*      Copyright 2000 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  GAPAPI - Stonestreet One Bluetooth Stack Generic Access Profile API       */
/*           Type Definitions, Constants, and Prototypes.                     */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   10/27/00  D. Lange       Initial creation.                               */
/*   12/07/07  D. Mason       Changes for BT 2.1                              */
/*   09/18/08  J. Toole       Updates for BT 2.1                              */
/******************************************************************************/
#ifndef __GAPAPIH__
#define __GAPAPIH__

#include "HCITypes.h"           /* HCI Related Type Definitions.              */
#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "BTTypes.h"            /* Bluetooth Type Definitions/Constants.      */

#include "BTPSCFG.h"            /* BTPS Configuration Constants.              */

   /* The following values represent the Time Limits (in seconds) that  */
   /* are allowable for the various Discoverability Modes.  These       */
   /* values are used with the GAP_Set_Discoverability_Mode() function. */
   /* * NOTE * There is a special value defined for a Time Limit of     */
   /*          Infinite for the General Discoverability Mode.           */
#define MINIMUM_DISCOVERABLE_MODE_TIME_LIMITED_DISCOVERABLE_MODE      (31L)
#define MAXIMUM_DISCOVERABLE_MODE_TIME_LIMITED_DISCOVERABLE_MODE      (60L)

#define MINIMUM_DISCOVERABLE_MODE_TIME_GENERAL_DISCOVERABLE_MODE      (31L)
#define MAXIMUM_DISCOVERABLE_MODE_TIME_GENERAL_DISCOVERABLE_MODE (60L*60L*24L)

#define INFINITE_DISCOVERABLE_MODE_TIME_GENERAL_DISCOVERABLE_MODE         0

   /* The following values represent the Time Limits (in seconds) that  */
   /* are allowable for the Input Time Limits for the Inquiry Process.  */
   /* These values are used with the GAP_Perform_Inquiry() function.    */
   /* * NOTE * If the Minimum and Maximum Inquiry Periods are used then */
   /*          the following equation *MUST* be satisfied:              */
   /*          Maximum Period Length >                                  */
   /*          Minimum Period Length >                                  */
   /*          Inquiry Length                                           */
#define MINIMUM_INQUIRY_LENGTH                                            2
#define MAXIMUM_INQUIRY_LENGTH                                           61

#define MINIMUM_MINIMUM_INQUIRY_PERIOD_LENGTH                          (3L)
#define MAXIMUM_MINIMUM_INQUIRY_PERIOD_LENGTH                      (83883L)

#define MINIMUM_MAXIMUM_INQUIRY_PERIOD_LENGTH                          (4L)
#define MAXIMUM_MAXIMUM_INQUIRY_PERIOD_LENGTH                      (83883L)

   /* The following values represent the Minimum and Maximum Number of  */
   /* actual Bluetooth Devices that are to be waited for during the     */
   /* Inquiry Process.  These values are used with the                  */
   /* GAP_Perform_Inquiry() function.                                   */
   /* * NOTE * There is a special value defined for an unlimited number */
   /*          of Inquiry Responses.                                    */
#define MINIMUM_NUMBER_INQUIRY_RESPONSES                                  1
#define MAXIMUM_NUMBER_INQUIRY_RESPONSES                                255

#define INFINITE_NUMBER_INQUIRY_RESPONSES                                 0

   /* The following enumerated type represents the supported Discovery  */
   /* Modes that a Bluetooth Device can be set to.  These types are     */
   /* used with the GAP_Set_Discoverability_Mode() and the              */
   /* GAP_Query_Discoverability_Mode() functions.                       */
typedef enum
{
   dmNonDiscoverableMode,
   dmLimitedDiscoverableMode,
   dmGeneralDiscoverableMode
} GAP_Discoverability_Mode_t;

   /* The following enumerated type represents the supported            */
   /* Connectability Modes that a Bluetooth Device can be set to.  These*/
   /* types are used with the GAP_Set_Connectability_Mode() and the     */
   /* GAP_Query_Connectability_Mode() functions.                        */
typedef enum
{
   cmNonConnectableMode,
   cmConnectableMode
} GAP_Connectability_Mode_t;

   /* The following enumerated type represents the supported            */
   /* Pairability Modes that a Bluetooth Device can be set to.  These   */
   /* types are used with the GAP_Set_Pairability_Mode() and the        */
   /* GAP_Query_Pairability_Mode() functions.                           */
typedef enum
{
  pmNonPairableMode,
  pmPairableMode,
  pmPairableMode_EnableSecureSimplePairing
} GAP_Pairability_Mode_t;

   /* The following enumerated type represents the supported            */
   /* Authentication Modes that a Bluetooth Device can be set to.  These*/
   /* types are used with the GAP_Set_Authentication_Mode() and the     */
   /* GAP_Query_Authentication_Mode() functions.                        */
typedef enum
{
   amDisabled,
   amEnabled
} GAP_Authentication_Mode_t;

   /* The following enumerated type represents the supported            */
   /* Encryption Modes that a Bluetooth Device can be set to.  These    */
   /* types are used with the GAP_Set_Encryption_Mode() and the         */
   /* GAP_Query_Encryption_Mode() functions.                            */
typedef enum
{
   emDisabled,
   emEnabled
} GAP_Encryption_Mode_t;

   /* The following enumerated type represents the supported Bonding    */
   /* Types that the Bluetooth Device can be instructed to perform.     */
   /* These types are used with the GAP_Initiate_Bonding() function.    */
typedef enum
{
   btGeneral,
   btDedicated
} GAP_Bonding_Type_t;

   /* The following enumerated type represents the supported Inquiry    */
   /* Types that can be used when performing an Inquiry Process of      */
   /* Bluetooth Device(s).  These types are used with the               */
   /* GAP_Perform_Inquiry() function.                                   */
typedef enum
{
   itGeneralInquiry,
   itLimitedInquiry
} GAP_Inquiry_Type_t;

   /* The following enumerated type represents the supported Inquiry    */
   /* Modes that a Bluetooth Device can be set to.  These types are used*/
   /* with the GAP_Set_Inquiry_Mode() and the GAP_Query_Inquiry_Mode()  */
   /* functions.                                                        */
typedef enum
{
   imStandard,
   imRSSI,
   imExtended
} GAP_Inquiry_Mode_t;

   /* The following enumerated type represents the GAP Event Reason     */
   /* (and valid Data) and is used with the GAP Event Callback.         */
typedef enum
{
   etInquiry_Result,
   etEncryption_Change_Result,
   etAuthentication,
   etRemote_Name_Result,
   etInquiry_Entry_Result,
   etInquiry_With_RSSI_Entry_Result,
   etExtended_Inquiry_Entry_Result,
   etEncryption_Refresh_Complete,
   etRemote_Features_Result,
   etRemote_Version_Information_Result
} GAP_Event_Type_t;

   /* The following type declaration defines an Individual Inquiry      */
   /* Result Entry.  This information forms the Data Portion of the     */
   /* GAP_Inquiry_Event_Data_t structure.                               */
typedef struct _tagGAP_Inquiry_Data_t
{
   BD_ADDR_t         BD_ADDR;
   Byte_t            Page_Scan_Repetition_Mode;
   Byte_t            Page_Scan_Period_Mode;
   Byte_t            Page_Scan_Mode;
   Class_of_Device_t Class_of_Device;
   Word_t            Clock_Offset;
} GAP_Inquiry_Data_t;

#define GAP_INQUIRY_DATA_SIZE                                     (sizeof(GAP_Inquiry_Data_t))

   /* The following type declaration defines the result of an Inquiry   */
   /* Process that was started via the GAP_Perform_Inquiry() function.  */
   /* The Number of Devices Entry defines the number of Inquiry Data    */
   /* Entries that the GAP_Inquiry_Data member points to (if non-zero). */
typedef struct _tagGAP_Inquiry_Event_Data_t
{
   Word_t              Number_Devices;
   GAP_Inquiry_Data_t *GAP_Inquiry_Data;
} GAP_Inquiry_Event_Data_t;

#define GAP_INQUIRY_EVENT_DATA_SIZE                               (sizeof(GAP_Inquiry_Event_Data_t))

   /* The following type declaration defines an individual result of an */
   /* Inquiry Process that was started via the GAP_Perform_Inquiry()    */
   /* function.  This event data is generated for each Inquiry Result as*/
   /* it is received.                                                   */
typedef struct _tagGAP_Inquiry_Entry_Event_Data_t
{
   BD_ADDR_t         BD_ADDR;
   Byte_t            Page_Scan_Repetition_Mode;
   Byte_t            Page_Scan_Period_Mode;
   Byte_t            Page_Scan_Mode;
   Class_of_Device_t Class_of_Device;
   Word_t            Clock_Offset;
} GAP_Inquiry_Entry_Event_Data_t;

#define GAP_INQUIRY_ENTRY_EVENT_DATA_SIZE                         (sizeof(GAP_Inquiry_Entry_Event_Data_t))

   /* The following type declaration defines an Individual Inquiry      */
   /* Result with RSSI Entry.  This information forms the Data Portion  */
   /* of the GAP_Inquiry_Event_Data_t structure.                        */
typedef struct _tagGAP_Inquiry_With_RSSI_Entry_Event_Data_t
{
   BD_ADDR_t         BD_ADDR;
   Byte_t            Page_Scan_Repetition_Mode;
   Byte_t            Page_Scan_Period_Mode;
   Class_of_Device_t Class_of_Device;
   Word_t            Clock_Offset;
   Byte_t            RSSI;
} GAP_Inquiry_With_RSSI_Entry_Event_Data_t;

#define GAP_INQUIRY_WITH_RSSI_ENTRY_EVENT_DATA_SIZE               (sizeof(GAP_Inquiry_With_RSSI_Entry_Event_Data_t))

   /* The following structure defines an individual Extended Inquiry    */
   /* Response Structure Entry that is present in an Extended Inquiry   */
   /* Response Data field.  This structure is used with the             */
   /* GAP_Extended_Inquiry_Response_Data_t container structure so that  */
   /* individual entries can be accessed in a convienant, array-like,   */
   /* form.  The first member specifies the Extended Inquiry Response   */
   /* Data type (These types are of the form                            */
   /* HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_xxx, where 'xxx'          */
   /* is the individual Data type).  The second member specifies the    */
   /* length of data that is pointed to by the third member.  The       */
   /* third member points to the actual data for the individual entry   */
   /* (length is specified by the second member).                       */
   /* * NOTE * The Data_Type member is defined in the specification to  */
   /*          be variable length.  The current specification does not  */
   /*          utilize this member in this way (they are all defined to */
   /*          be a single octet, currently).                           */
typedef struct _tagGAP_Extended_Inquiry_Response_Data_Entry_t
{
   DWord_t  Data_Type;
   Byte_t   Data_Length;
   Byte_t  *Data_Buffer;
} GAP_Extended_Inquiry_Response_Data_Entry_t;

#define GAP_EXTENDED_INQUIRY_RESPONSE_DATA_ENTRY_SIZE             (sizeof(GAP_Extended_Inquiry_Response_Data_Entry_t))

   /* The following structure is a container structure that is used to  */
   /* represent all the entries in an Extended Inquiry Response Data    */
   /* Field.  This structure is used so that all fields are easy to     */
   /* parse and access (i.e. there are no MACRO's required to access    */
   /* variable length records).  The first member of this structure     */
   /* specifies how many individual entries are contained in the        */
   /* Extended Inquiry Response Data structure.  The second member is a */
   /* pointer to an array that contains each individual entry of the    */
   /* Extended Inquiry Response Data structure (note that the number of */
   /* individual entries pointed to by this array will be specified by  */
   /* the Number_Entries member (first member).                         */
typedef struct _tagGAP_Extended_Inquiry_Response_Data_t
{
   DWord_t                                     Number_Data_Entries;
   GAP_Extended_Inquiry_Response_Data_Entry_t *Data_Entries;
} GAP_Extended_Inquiry_Response_Data_t;

#define GAP_EXTENDED_INQUIRY_RESPONSE_DATA_SIZE                   (sizeof(GAP_Extended_Inquiry_Response_Data_t))

   /* The following type declaration defines an Extended Inquiry Result */
   /* This information forms the Data Portion of the                    */
   /* GAP_Extended_Inquiry_Entry_Event_Data_t structure.                */
typedef struct _tagGAP_Extended_Inquiry_Entry_Event_Data_t
{
   BD_ADDR_t                             BD_ADDR;
   Byte_t                                Page_Scan_Repetition_Mode;
   Byte_t                                Reserved;
   Class_of_Device_t                     Class_of_Device;
   Word_t                                Clock_Offset;
   Byte_t                                RSSI;
   GAP_Extended_Inquiry_Response_Data_t  Extended_Inquiry_Response_Data;
   Extended_Inquiry_Response_Data_t     *Raw_Extended_Inquiry_Response_Data;
} GAP_Extended_Inquiry_Entry_Event_Data_t;

#define GAP_EXTENDED_INQUIRY_ENTRY_EVENT_DATA_SIZE                (sizeof(GAP_Extended_Inquiry_Entry_Event_Data_t))

   /* The following type declaration defines GAP Encryption Status      */
   /* Information that is used with the GAP Encryption Change Result    */
   /* Event.                                                            */
typedef struct _tagGAP_Encryption_Mode_Event_Data_t
{
   BD_ADDR_t             Remote_Device;
   Byte_t                Encryption_Change_Status;
   GAP_Encryption_Mode_t Encryption_Mode;
} GAP_Encryption_Mode_Event_Data_t;

#define GAP_ENCRYPTION_MODE_EVENT_DATA_SIZE                       (sizeof(GAP_Encryption_Mode_Event_Data_t))

   /* The following enumerated type represents currently defined        */
   /* Keypress actions that can be specified with the authentication    */
   /* events.                                                           */
typedef enum
{
   kpEntryStarted,
   kpDigitEntered,
   kpDigitErased,
   kpCleared,
   kpEntryCompleted
} GAP_Keypress_t;

   /* The following enumerated type is used with the                    */
   /* GAP_IO_Capabilities_t structure and Bonding functions to specify  */
   /* the Bonding requirements of the Authentication Requirements of the*/
   /* I/O Capability Request Reply Event.                               */
typedef enum
{
   ibNoBonding,
   ibDedicatedBonding,
   ibGeneralBonding
} GAP_IO_Capability_Bonding_Type_t;

   /* The following represent the defined I/O Capabilities that can be  */
   /* specified/utilized by this module.                                */
   /* Display Only         - The device only has a display with no      */
   /*                        input capability.                          */
   /* Display with Yes/No  - The device has both a display and the      */
   /*                        ability for a user to enter yes/no, either */
   /*                        through a single keypress or via a Keypad  */
   /* Keyboard Only        - The device has no display capability, but  */
   /*                        does have a single key or keypad.          */
   /* No Input nor Output  - The device has no input and no output. A   */
   /*                        device such as this may use Out of Band or */
   /*                        Just Works associations.                   */
typedef enum
{
   icDisplayOnly,
   icDisplayYesNo,
   icKeyboardOnly,
   icNoInputNoOutput
} GAP_IO_Capability_t;

   /* The following constant represents the maximum number of digits    */
   /* that can can be specified for a Pass Key.                         */
#define GAP_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS                      (HCI_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS)

   /* The following constants represent the minimum and maximum values  */
   /* that are valid for Pass Keys.                                     */
#define GAP_PASSKEY_MINIMUM_VALUE                                 (HCI_PASSKEY_NUMERIC_VALUE_MINIMUM)
#define GAP_PASSKEY_MAXIMUM_VALUE                                 (HCI_PASSKEY_NUMERIC_VALUE_MAXIMUM)

   /* The following type declaration defines GAP Encryption Refresh     */
   /* Information that is used with the GAP Encryption Refresh Result   */
   /* Event.                                                            */
typedef struct _tagGAP_Encryption_Refresh_Complete_Event_Data_t
{
   BD_ADDR_t Remote_Device;
   Byte_t    Status;
} GAP_Encryption_Refresh_Complete_Event_Data_t;

#define GAP_ENCRYPTION_REFRESH_COMPLETE_EVENT_DATA_SIZE           (sizeof(GAP_Encryption_Refresh_Complete_Event_Data_t))

   /* The following structure defines the Out of Band (OOB) Data that is*/
   /* exchanged during the Out of Band Authentication process.          */
typedef struct _tagGAP_Out_Of_Band_Data_t
{
   Simple_Pairing_Hash_t       Simple_Pairing_Hash;
   Simple_Pairing_Randomizer_t Simple_Pairing_Randomizer;
} GAP_Out_Of_Band_Data_t;

#define GAP_OUT_OF_BAND_DATA_SIZE                                 (sizeof(GAP_Out_Of_Band_Data_t))

   /* The following structure defines the I/O Capabilities supported    */
   /* during Capablities exchange during the Authentication Process     */
   /* (required during Secure Simple Pairing).                          */
typedef struct _tagGAP_IO_Capabilities_t
{
   GAP_IO_Capability_t              IO_Capability;
   Boolean_t                        OOB_Data_Present;
   Boolean_t                        MITM_Protection_Required;
   GAP_IO_Capability_Bonding_Type_t Bonding_Type;
} GAP_IO_Capabilities_t;

#define GAP_IO_CAPABILITIES_SIZE                                  (sizeof(GAP_IO_Capabilities_t))

   /* The following enumerated type is used with the Authentication     */
   /* Event Data Structure and defines the reason that the              */
   /* Authentication Callback was issued, this defines what data in the */
   /* structure is pertinant.                                           */
typedef enum
{
   atLinkKeyRequest,
   atPINCodeRequest,
   atAuthenticationStatus,
   atLinkKeyCreation,
   atIOCapabilityRequest,
   atUserConfirmationRequest,
   atPasskeyRequest,
   atPasskeyNotification,
   atKeypressNotification,
   atRemoteOutOfBandDataRequest,
   atIOCapabilityResponse
} GAP_Authentication_Event_Type_t;

   /* The following enumerated type represents the different            */
   /* Authentication Methods that can be used.                          */
typedef enum
{
   atLinkKey,
   atPINCode,
   atUserConfirmation,
   atPassKey,
   atKeypress,
   atOutOfBandData,
   atIOCapabilities
} GAP_Authentication_Type_t;

   /* The following type declaration defines GAP Authentication         */
   /* Information that can be set and/or returned.  The first member    */
   /* of this structure specifies which Data Member should be used.     */
   /* * NOTE * For GAP Authentication Types that are rejections, the    */
   /*          Authentication_Data_Length member is set to zero and     */
   /*          All Data Members can be ignored (since non are valid).   */
   /* * NOTE * Currently the Bonding_Type member of the IO_Capabilities */
   /*          member is ignored.  The correct value is calculated and  */
   /*          inserted automatically.                                  */
typedef struct _tagGAP_Authentication_Information_t
{
   GAP_Authentication_Type_t GAP_Authentication_Type;
   Byte_t                    Authentication_Data_Length;
   union
   {
      PIN_Code_t             PIN_Code;
      Link_Key_t             Link_Key;
      Boolean_t              Confirmation;
      DWord_t                Passkey;
      GAP_Keypress_t         Keypress;
      GAP_Out_Of_Band_Data_t Out_Of_Band_Data;
      GAP_IO_Capabilities_t  IO_Capabilities;
   } Authentication_Data;
} GAP_Authentication_Information_t;

#define GAP_AUTHENTICATION_INFORMATION_SIZE                       (sizeof(GAP_Authentication_Information_t))

   /* The following type declaration specifies the Link Key information */
   /* included in a GAP_Authentication_Event_Data structure when the    */
   /* GAP_Authentication_Event_Type = atLinkKeyCreation.                */
typedef struct _tagGAP_Authentication_Event_Link_Key_Info_t
{
   Link_Key_t Link_Key;
   Byte_t     Key_Type;
} GAP_Authentication_Event_Link_Key_Info_t;

#define GAP_AUTHENTICATION_EVENT_LINK_KEY_INFO_SIZE               (sizeof(GAP_Authentication_Event_Link_Key_Info_t))

   /* The following type declaration specifies the information that can */
   /* be returned in a GAP_Authentication_Callback.  This information   */
   /* is passed to the Callback when a GAP_Authentication Callback is   */
   /* issued.  The first member of this structure specifies which Data  */
   /* Member is valid.  Currently the following members are valid for   */
   /* the following values of the GAP_Authentication_Event_Type member: */
   /*                                                                   */
   /*    atLinkKeyRequest             - No Further Data.                */
   /*    atPINCodeRequest             - No Further Data.                */
   /*    atAuthenticationStatus       - Authentication_Status is valid. */
   /*    atLinkKeyCreation            - Link_Key_Info is valid.         */
   /*    atKeypressNotification       - Keypress_Type is valid.         */
   /*    atUserConfirmationRequest    - Numeric_Value is valid.         */
   /*    atPasskeyNotification        - Numeric_Value is valid.         */
   /*    atPasskeyRequest             - No Further Data.                */
   /*    atRemoteOutOfBandDataRequest - No Further Data.                */
   /*    atIOCapabilityRequest        - No Further Data.                */
   /*    atIOCapabilityResponse       - IO_Capabilities is valid.       */
   /*                                                                   */
typedef struct _tagGAP_Authentication_Event_Data_t
{
   GAP_Authentication_Event_Type_t GAP_Authentication_Event_Type;
   BD_ADDR_t                       Remote_Device;
   union
   {
      Byte_t                                   Authentication_Status;
      GAP_Authentication_Event_Link_Key_Info_t Link_Key_Info;
      DWord_t                                  Numeric_Value;
      GAP_Keypress_t                           Keypress_Type;
      GAP_IO_Capabilities_t                    IO_Capabilities;
   } Authentication_Event_Data;
} GAP_Authentication_Event_Data_t;

#define GAP_AUTHENTICATION_EVENT_DATA_SIZE                        (sizeof(GAP_Authentication_Event_Data_t))

   /* The following structure represents the GAP Remote Name Response   */
   /* Event Data that is returned from the                              */
   /* GAP_Query_Remote_Device_Name() function.  The Remote_Name         */
   /* member will point to a NULL terminated string that represents     */
   /* the User Friendly Bluetooth Name of the Remote Device associated  */
   /* with the specified BD_ADDR.                                       */
typedef struct _tagGAP_Remote_Name_Event_Data_t
{
   Byte_t     Remote_Name_Status;
   BD_ADDR_t  Remote_Device;
   char      *Remote_Name;
} GAP_Remote_Name_Event_Data_t;

#define GAP_REMOTE_NAME_EVENT_DATA_SIZE                           (sizeof(GAP_Remote_Name_Event_Data_t))

   /* The following structure represents the GAP Remote Features        */
   /* Response Event Data that is returned from the                     */
   /* GAP_Query_Remote_Features() function.                             */
typedef struct _tagGAP_Remote_Features_Event_Data_t
{
   Byte_t         Status;
   BD_ADDR_t      BD_ADDR;
   LMP_Features_t Features;
   Byte_t         Page_Number;
   Byte_t         Maximum_Page_Number;
} GAP_Remote_Features_Event_Data_t;

#define GAP_REMOTE_FEATURES_EVENT_DATA_SIZE                       (sizeof(GAP_Remote_Features_Event_Data_t))

   /* The following structure represents the GAP Remote Features        */
   /* Response Event Data that is returned from the                     */
   /* GAP_Query_Remote_Features() function.                             */
typedef struct _tagGAP_Remote_Version_Information_Event_Data_t
{
   Byte_t         Status;
   BD_ADDR_t      BD_ADDR;
   Byte_t         LMP_Version;
   Word_t         Manufacturer_ID;
   Word_t         LMP_Subversion;
} GAP_Remote_Version_Information_Event_Data_t;

#define GAP_REMOTE_VERSION_INFORMATION_EVENT_DATA_SIZE            (sizeof(GAP_Remote_Version_Information_Event_Data_t))

   /* The following structure represents the container structure that   */
   /* holds all GAP Event Data Data.                                    */
typedef struct _tagGAP_Event_Data_t
{
   GAP_Event_Type_t Event_Data_Type;
   Word_t           Event_Data_Size;
   union
   {
      GAP_Inquiry_Event_Data_t                     *GAP_Inquiry_Event_Data;
      GAP_Encryption_Mode_Event_Data_t             *GAP_Encryption_Mode_Event_Data;
      GAP_Authentication_Event_Data_t              *GAP_Authentication_Event_Data;
      GAP_Remote_Name_Event_Data_t                 *GAP_Remote_Name_Event_Data;
      GAP_Inquiry_Entry_Event_Data_t               *GAP_Inquiry_Entry_Event_Data;
      GAP_Inquiry_With_RSSI_Entry_Event_Data_t     *GAP_Inquiry_With_RSSI_Entry_Event_Data;
      GAP_Extended_Inquiry_Entry_Event_Data_t      *GAP_Extended_Inquiry_Entry_Event_Data;
      GAP_Encryption_Refresh_Complete_Event_Data_t *GAP_Encryption_Refresh_Complete_Event_Data;
      GAP_Remote_Features_Event_Data_t             *GAP_Remote_Features_Event_Data;
      GAP_Remote_Version_Information_Event_Data_t  *GAP_Remote_Version_Information_Event_Data;
   } Event_Data;
} GAP_Event_Data_t;

#define GAP_EVENT_DATA_SIZE                                       (sizeof(GAP_Event_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* the GAP Event Receive Data Callback.  This function will be called*/
   /* whenever a Callback has been registered for the specified GAP     */
   /* Action that is associated with the specified Bluetooth Stack ID.  */
   /* This function passes to the caller the Bluetooth Stack ID, the    */
   /* GAP Event Data of the specified Event, and the GAP Event Callback */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GAP Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e. this  function DOES NOT have be          */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that    */
   /* the User does NOT own.  Therefore, processing in this function    */
   /* should be as efficient as possible (this argument holds anyway    */
   /* because other GAP Events will not be processed while this         */
   /* function call is outstanding).                                    */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving other GAP Events.   */
   /*            A Deadlock WILL occur because NO GAP Event             */
   /*            Callbacks will be issued while this function is        */
   /*            currently outstanding.                                 */
typedef void (BTPSAPI *GAP_Event_Callback_t)(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);

   /* The following function is provided to set the Discoverability Mode*/
   /* of the Local Bluetooth Device specified by the Bluetooth Protocol */
   /* Stack that is specified by the Bluetooth Protocol Stack ID.  The  */
   /* second parameter specifies the Discoverability Mode to place the  */
   /* Local Bluetooth Device into, and the third parameter species the  */
   /* length of time (in Seconds) that the Local Bluetooth Device is    */
   /* to be placed into the specified Discoverable Mode.  At the end of */
   /* this time (provided the time is NOT Infinite), the Local Bluetooth*/
   /* Device will return to NON-Discoverable Mode.  This function       */
   /* returns zero if the Discoverability Mode was able to be           */
   /* successfully changed, otherwise this function returns a negative  */
   /* value which signifies an error condition.                         */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Discoverability_Mode(unsigned int BluetoothStackID, GAP_Discoverability_Mode_t GAP_Discoverability_Mode, unsigned long Max_Discoverable_Time);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Discoverability_Mode_t)(unsigned int BluetoothStackID, GAP_Discoverability_Mode_t GAP_Discoverability_Mode, unsigned long Max_Discoverable_Time);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Discoverability Mode Parameters for the Bluetooth Device  */
   /* that is specified by the Bluetooth Protocol Stack that is         */
   /* associated with the specified Bluetooth Stack ID.  The second     */
   /* parameter to this function is a pointer to a variable that will   */
   /* receive the current Discoverability Mode of the Bluetooth Device, */
   /* and the Last parameter needs to be a pointer to a variable that   */
   /* will receive the current Discoverability Mode Maximum             */
   /* Discoverability Mode Timeout value.  Both of these parameters     */
   /* must be valid (i.e. NON-NULL), and upon successful completion     */
   /* of this function will contain the current Discoverability Mode    */
   /* of the Local Bluetooth Device.  This function will return zero on */
   /* success, or a negative return error code if there was an error.   */
   /* If this function returns success, then the GAP Discoverability    */
   /* Mode will be valid and the Maximum Discoverable Time will be      */
   /* valid as well.                                                    */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Discoverability_Mode(unsigned int BluetoothStackID, GAP_Discoverability_Mode_t *GAP_Discoverability_Mode, unsigned long *Max_Discoverable_Time);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Discoverability_Mode_t)(unsigned int BluetoothStackID, GAP_Discoverability_Mode_t *GAP_Discoverability_Mode, unsigned long *Max_Discoverable_Time);
#endif

   /* The following function is provided to set the Connectability Mode */
   /* of the Local Bluetooth Device specified by the Bluetooth Protocol */
   /* Stack that is specified by the Bluetooth Protocol Stack ID.  The  */
   /* second parameter specifies the Connectability Mode to place the   */
   /* Local Bluetooth Device into.  This function returns zero if the   */
   /* Connectability Mode was able to be successfully changed, otherwise*/
   /* this function returns a negative value which signifies an error   */
   /* condition.                                                        */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Connectability_Mode(unsigned int BluetoothStackID, GAP_Connectability_Mode_t GAP_Connectability_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Connectability_Mode_t)(unsigned int BluetoothStackID, GAP_Connectability_Mode_t GAP_Connectability_Mode);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Connectability Mode Setting for the Bluetooth Device that */
   /* is specified by the Bluetooth Protocol Stack that is associated   */
   /* with the specified Bluetooth Stack ID.  The second parameter to   */
   /* this function is a pointer to a variable that will receive the    */
   /* current Connectability Mode of the Bluetooth Device.  The second  */
   /* parameter must be valid (i.e. NON-NULL) and upon successful       */
   /* completion of this function will contain the current              */
   /* Connectability Mode of the Local Bluetooth Device.  This function */
   /* will return zero on success, or a negative return error code if   */
   /* there was an error.  If this function returns success, then the   */
   /* GAP Connectability Mode will contain the current Connectability   */
   /* Mode.                                                             */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Connectability_Mode(unsigned int BluetoothStackID, GAP_Connectability_Mode_t *GAP_Connectability_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Connectability_Mode_t)(unsigned int BluetoothStackID, GAP_Connectability_Mode_t *GAP_Connectability_Mode);
#endif

   /* The following function is provided to set the Pairability Mode of */
   /* the Local Bluetooth Device specified by the Bluetooth Protocol    */
   /* Stack that is specified by the Bluetooth Protocol Stack ID.  The  */
   /* second parameter specifies the Pairability Mode to place the Local*/
   /* Bluetooth Device into.  This function returns zero if the         */
   /* Pairability Mode was able to be successfully changed, otherwise   */
   /* this function returns a negative value which signifies an error   */
   /* condition.                                                        */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Pairability_Mode(unsigned int BluetoothStackID, GAP_Pairability_Mode_t GAP_Pairability_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Pairability_Mode_t)(unsigned int BluetoothStackID, GAP_Pairability_Mode_t GAP_Pairability_Mode);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Pairability Mode Settings for the Bluetooth Device that   */
   /* is specified by the Bluetooth Protocol Stack that is associated   */
   /* with the specified Bluetooth Stack ID.  The second parameter to   */
   /* this function is a pointer to a variable that will receive the    */
   /* current Pairability Mode of the Bluetooth Device.  The second     */
   /* parameter must be valid (i.e. NON-NULL) and upon successful       */
   /* completion of this function will contain the current Pairability  */
   /* Mode of the Local Bluetooth Device.  This function will return    */
   /* zero on success, or a negative return error code if there was an  */
   /* error.  If this function returns success, then the GAP Pairability*/
   /* Mode will contain the current Pairability Mode.                   */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Pairability_Mode(unsigned int BluetoothStackID, GAP_Pairability_Mode_t *GAP_Pairability_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Pairability_Mode_t)(unsigned int BluetoothStackID, GAP_Pairability_Mode_t *GAP_Pairability_Mode);
#endif

   /* The following function is provided to set the Authentication Mode */
   /* of the Local Bluetooth Device specified by the Bluetooth Protocol */
   /* Stack that is specified by the Bluetooth Protocol Stack ID.  The  */
   /* second parameter specifies the Authentication Mode to place the   */
   /* Local Bluetooth Device into.  This function returns zero if the   */
   /* Authentication Mode was able to be successfully changed, otherwise*/
   /* this function returns a negative value which signifies an error   */
   /* condition.                                                        */
   /* * NOTE * If Authentication is enabled for the Local Bluetooth     */
   /*          Device, then this Means that EVERY Connection (both      */
   /*          incoming and outgoing) will require Authentication at    */
   /*          the Link Level.                                          */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Authentication_Mode(unsigned int BluetoothStackID, GAP_Authentication_Mode_t GAP_Authentication_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Authentication_Mode_t)(unsigned int BluetoothStackID, GAP_Authentication_Mode_t GAP_Authentication_Mode);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Authentication Mode Settings for the Bluetooth Device     */
   /* that is specified by the Bluetooth Protocol Stack that is         */
   /* associated with the specified Bluetooth Stack ID.  The second     */
   /* parameter to this function is a pointer to a variable that will   */
   /* receive the current Authentication Mode of the Bluetooth Device.  */
   /* The second parameter must be valid (i.e. NON-NULL) and upon       */
   /* successful completion of this function will contain the current   */
   /* Authentication Mode of the Local Bluetooth Device.  This function */
   /* will return zero on success, or a negative return error code if   */
   /* there was an error.  If this function returns success, then the   */
   /* GAP Authentication Mode will be contain the current Authentication*/
   /* Mode Settings.                                                    */
   /* * NOTE * If Authentication is enabled for the Local Bluetooth     */
   /*          Device, then this Means that EVERY Connection (both      */
   /*          incoming and outgoing) will require Authentication at    */
   /*          the Link Level.                                          */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Authentication_Mode(unsigned int BluetoothStackID, GAP_Authentication_Mode_t *GAP_Authentication_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Authentication_Mode_t)(unsigned int BluetoothStackID, GAP_Authentication_Mode_t *GAP_Authentication_Mode);
#endif

   /* The following function is provided to allow the Setting of        */
   /* Encryption Modes for the Local Bluetooth Device or to             */
   /* Enable/Disable Encryption for the specified Bluetooth Board       */
   /* Address.  The first parameter specifies the Bluetooth Protocol    */
   /* Stack of the Local Bluetooth Device.  The second parameter        */
   /* specifies the Bluetooth Board Address to apply the Encryption     */
   /* Mode Setting to (could be local or remote).  The Third parameter  */
   /* specifies the state of the Encryption to change to.  The final    */
   /* two parameters specify the GAP Encryption Status Callback to call */
   /* when the Encryption is changed.  This callback will contain the   */
   /* actual status of the Encryption Change (success or failure).      */
   /* This function returns zero if the Encryption Mode was changed     */
   /* (see note below), or a negative return value which signifies an   */
   /* error condition.                                                  */
   /* * NOTE * If the Local Board Address is specified for the second   */
   /*          parameter, then this function will set the specified     */
   /*          Encryption Mode for ALL further Link Level Connections.  */
   /*          When the Local Board Address is specified, the Callback  */
   /*          Function and Parameter are ignored, and the function     */
   /*          return value indicates whether or not the Encryption     */
   /*          Change was successful (for the Local Device).            */
   /* * NOTE * If the second parameter is NOT the Local Board Address,  */
   /*          then this function will set the Encryption Mode on the   */
   /*          Link Level for the specified Bluetooth Link.  A Physical */
   /*          ACL Link MUST already exist for this to work.  The       */
   /*          actual status of the Encryption Change for this link     */
   /*          will be passed to the Callback Information that is       */
   /*          required when using this function in this capacity.      */
   /* * NOTE * Because this function is asynchronous in nature (when    */
   /*          specifying a remote BD_ADDR), this function will notify  */
   /*          the caller of the result via the installed Callback.  The*/
   /*          caller is free to cancel the Encryption Mode Change at   */
   /*          any time by issuing the GAP_Cancel_Set_Encryption_Mode() */
   /*          function and specifying the BD_ADDR of the Bluetooth     */
   /*          Device that was specified in this call.  It should be    */
   /*          noted that when the Callback is cancelled, the Callback  */
   /*          is the ONLY thing that is cancelled (i.e. the GAP module */
   /*          still changes the Encryption for the Link, it's just that*/
   /*          NO Callback is issued).                                  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Encryption_Mode(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Encryption_Mode_t GAP_Encryption_Mode, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Encryption_Mode_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Encryption_Mode_t GAP_Encryption_Mode, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to cancel the future calling   */
   /* of an Encryption Mode Callback that was installed via a           */
   /* successful call to the GAP_Set_Encryption_Mode() function.  This  */
   /* function DOES NOT cancel the changing of the Encryption Mode for  */
   /* the specified Bluetooth Device, it ONLY cancels the Callback      */
   /* Notification.  This function accepts as input the Bluetooth       */
   /* Protocol Stack ID of the Bluetooth Device used when the           */
   /* GAP_Set_Encryption_Mode() function was previously issued, and the */
   /* Board Address of the Bluetooth Device that the previous call was  */
   /* called with.  The BD_ADDR parameter MUST be valid, and cannot be  */
   /* the BD_ADDR of the Local Bluetooth Device because the Local       */
   /* Encryption Mode Change does not use the Callback mechanism.  This */
   /* function returns zero if successful, or a negative return error   */
   /* code if the function was unsuccessful.                            */
BTPSAPI_DECLARATION int BTPSAPI GAP_Cancel_Set_Encryption_Mode(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Cancel_Set_Encryption_Mode_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Encryption Mode Parameters for the Bluetooth Device that  */
   /* is specified by the Bluetooth Protocol Stack that is associated   */
   /* with the specified Bluetooth Stack ID.  The second parameter to   */
   /* this function is the Bluetooth Board Address of the Link to       */
   /* query the Encryption State of.  If the Local Bluetooth Board      */
   /* Address is specified for this parameter then the Encryption       */
   /* Information that is returned represents the Current Encryption    */
   /* Link Level State of all future ACL Connections (both incoming and */
   /* outgoing).  The third parameter to this function is a pointer to  */
   /* a variable that will receive the current Encryption Mode of the   */
   /* Bluetooth Device/Bluetooth Link.  The third parameter to this     */
   /* function must be valid (i.e. NON-NULL) and upon successful        */
   /* completion of this function will contain the current Encryption   */
   /* Mode for the Bluetooth Device/Link Requested.  This function will */
   /* return zero on success, or a negative return error code if there  */
   /* was an error.  If this function returns success, then the GAP     */
   /* Encryption Mode parameter will contain the current Encryption     */
   /* Mode Setting for the specified Bluetooth Device/Bluetooth Link.   */
   /* * NOTE * If the Local Board Address is specified for the second   */
   /*          parameter, then this function will query the specified   */
   /*          Encryption Mode for ALL future Link Level Connections.   */
   /* * NOTE * If the second parameter is NOT the Local Board Address,  */
   /*          then this function will query the Encryption Mode on the */
   /*          Link Level for the specified Bluetooth Link.  A physical */
   /*          ACL Link MUST already exist for this to work.            */
   /* * NOTE * If the local Bluetooth Radio version is 2.1, then this   */
   /*          function will return an error since this function has    */
   /*          been deprecated for Bluetooth 2.1 and beyond. The HCI    */
   /*          will actually return an error if you were to invoke this */
   /*          command, but that could add an unnecessary delay that    */
   /*          be prevented by querying the current version in memory.  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Encryption_Mode(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Encryption_Mode_t *GAP_Encryption_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Encryption_Mode_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Encryption_Mode_t *GAP_Encryption_Mode);
#endif

   /* The following function is provided to allow a means to            */
   /* Authenticate a Remote Device.  This function accepts as input the */
   /* Bluetooth Protocol Stack ID of the Local Bluetooth Device, the    */
   /* Bluetooth Board Address of the Remote Device to Authenticate, and */
   /* the GAP Event Callback (and Callback Parameter) information that  */
   /* is to be used during the Authentication Process to inform the     */
   /* caller of Authentication Events and/or Requests.  This function   */
   /* returns zero if successful or a negative return error code if     */
   /* there was an error.  Note that even if this function returns      */
   /* success, it does NOT mean that the specified Remote Device was    */
   /* successfully Authenticated, it only means that the Authentication */
   /* Process has been started.                                         */
   /* * NOTE * Because this function is asynchronous in nature, this    */
   /*          function will notify the caller of the result via the    */
   /*          installed Callback.  The caller is free to cancel the    */
   /*          Authentication Process at any time by calling the        */
   /*          GAP_Cancel_Authenticate_Remote_Device() function and     */
   /*          specifying the BD_ADDR of the Bluetooth Device that was  */
   /*          specified in this call.  It should be noted that when    */
   /*          the Callback is cancelled, the Callback is the ONLY      */
   /*          thing that is cancelled (i.e. the GAP module still       */
   /*          processes the Authentication Events only NO Callback(s)  */
   /*          are issued).                                             */
BTPSAPI_DECLARATION int BTPSAPI GAP_Authenticate_Remote_Device(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Authenticate_Remote_Device_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism for       */
   /* Cancelling a Remote Device Authentication Process that was        */
   /* successfully initiated via a successful call to the               */
   /* GAP_Authenticate_Remote_Device() function.  This function accepts */
   /* as input the Bluetooth Protocol Stack ID of the Bluetooth Device  */
   /* that the Authentication Process was successfully started on and   */
   /* the Bluetooth Board Address of the Remote Device that was         */
   /* specified earlier (to be Authenticated).  This function returns   */
   /* zero if the Callback was successfully removed, or a negative      */
   /* return error code if there was an error.                          */
   /* * NOTE * Calling this function does NOT terminate the Remote      */
   /*          Authentication Process !!!!!  It only suspends further   */
   /*          Authentication Events for the GAP Event Callback that    */
   /*          was specified in the call of the original                */
   /*          GAP_Authenticate_Remote_Device() function.               */
BTPSAPI_DECLARATION int BTPSAPI GAP_Cancel_Authenticate_Remote_Device(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Cancel_Authenticate_Remote_Device_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is provided to allow a means to Register   */
   /* a GAP Event Callback to accept Remote Authentication Requests.    */
   /* This function accepts as input the Bluetooth Protocol Stack ID of */
   /* the Bluetooth Device and the GAP Event Callback Information to    */
   /* Register.  This function returns zero if the Callback was         */
   /* successfully installed, or a negative error return code if the    */
   /* Callback was not successfully installed.  It should be noted that */
   /* ONLY ONE Remote Authentication Callback can be installed per      */
   /* Bluetooth Device.  The Caller can Un-Register the Remote          */
   /* Authentication Callback that was registered with this function    */
   /* (if successful) by calling the                                    */
   /* GAP_Un_Register_Remote_Authentication() function.                 */
BTPSAPI_DECLARATION int BTPSAPI GAP_Register_Remote_Authentication(unsigned int BluetoothStackID, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Register_Remote_Authentication_t)(unsigned int BluetoothStackID, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* Un-Register a previously registered GAP Event Callback for        */
   /* Remote Authentication Events.  This function accepts as input the */
   /* Bluetooth Stack ID of the Bluetooth Device that a Remote          */
   /* Authentication Callback was registered previously (via a          */
   /* successful call to the GAP_Register_Remote_Authentication()       */
   /* function).  This function returns zero if successful, or a        */
   /* negative return error code if unsuccessful.                       */
BTPSAPI_DECLARATION int BTPSAPI GAP_Un_Register_Remote_Authentication(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Un_Register_Remote_Authentication_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided to allow a mechanism for the   */
   /* Local Device to Respond to GAP Authentication Events.  This       */
   /* function is used to set the Authentication Information for the    */
   /* specified Bluetooth Device.  This function accepts as input, the  */
   /* Bluetooth Protocol Stack ID of the Bluetooth Device that has      */
   /* requested the Authentication Request Events and the               */
   /* Authentication Response Information (specified by the caller).    */
   /* This function returns zero if successful, or a negative return    */
   /* error code if there was an error.                                 */
BTPSAPI_DECLARATION int BTPSAPI GAP_Authentication_Response(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Authentication_Information_t *GAP_Authentication_Information);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Authentication_Response_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Authentication_Information_t *GAP_Authentication_Information);
#endif

   /* The following function is provided to allow a mechanism for       */
   /* Starting an Inquiry Scan Procedure.  The first parameter to this  */
   /* function is the Bluetooth Protocol Stack of the Bluetooth Device  */
   /* that is to perform the Inquiry.  The second parameter is the Type */
   /* of Inquiry to perform.  The third and fourth parameters are the   */
   /* Minimum and Maximum Period Lengths (only valid in case a Periodic */
   /* Inquiry is to be performed).  The fifth parameter is the Length   */
   /* of Time to perform the Inquiry.  The sixth parameter is the       */
   /* Number of Responses to Wait for.  The final two parameters        */
   /* represent the Callback Function (and parameter) that is to be     */
   /* called when the specified Inquiry has completed.  This function   */
   /* returns zero if successful, or a negative return error code if    */
   /* an Inquiry was unable to be performed.                            */
   /* * NOTE * Only ONE Inquiry can be performed at any given time.     */
   /*          Calling this function while an outstanding Inquiry is    */
   /*          in progress will fail.  The caller can call the          */
   /*          GAP_Cancel_Inquiry() function to cancel a currently      */
   /*          executing Inquiry procedure.                             */
   /* * NOTE * The Minimum and Maximum Inquiry Parameters are optional  */
   /*          and, if specified, represent the Minimum and Maximum     */
   /*          Periodic Inquiry Periods.  The caller should set BOTH    */
   /*          of these values to zero if a simple Inquiry procedure    */
   /*          is to be used (Non-Periodic).  If these two parameters   */
   /*          are specified, then then these two parameters must       */
   /*          satisfy the following formula:                           */
   /*                                                                   */
   /*          MaximumPeriodLength > MinimumPeriodLength > InquiryLength*/
   /*                                                                   */
   /* * NOTE * All Inquiry Period Time parameters are specified in      */
   /*          seconds.                                                 */
BTPSAPI_DECLARATION int BTPSAPI GAP_Perform_Inquiry(unsigned int BluetoothStackID, GAP_Inquiry_Type_t GAP_Inquiry_Type, unsigned long MinimumPeriodLength, unsigned long MaximumPeriodLength, unsigned int InquiryLength, unsigned int MaximumResponses, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Perform_Inquiry_t)(unsigned int BluetoothStackID, GAP_Inquiry_Type_t GAP_Inquiry_Type, unsigned long MinimumPeriodLength, unsigned long MaximumPeriodLength, unsigned int InquiryLength, unsigned int MaximumResponses, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a means of cancelling */
   /* an Inquiry Process that was started via a successful call to the  */
   /* GAP_Perform_Inquiry() function.  This function accepts as input   */
   /* the Bluetooth Protocol Stack that is associated with the          */
   /* Bluetooth Device that is currently performing an Inquiry          */
   /* Process.  This function returns zero if the Inquiry Process was   */
   /* able to be cancelled, or a negative return error code if there    */
   /* was an error.  If this function returns success then the GAP      */
   /* Callback that was installed with the GAP_Perform_Inquiry()        */
   /* function will NEVER be called.                                    */
BTPSAPI_DECLARATION int BTPSAPI GAP_Cancel_Inquiry(unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Cancel_Inquiry_t)(unsigned int BluetoothStackID);
#endif

   /* The following function is provided to set the Inquiry Mode of the */
   /* Local Bluetooth Device specified by the Bluetooth Protocol Stack  */
   /* that is specified by the Bluetooth Protocol Stack ID.  The second */
   /* parameter specifies the Inquiry Mode to place the Local Bluetooth */
   /* Device into.  This function returns zero if the Inquiry Mode was  */
   /* able to be successfully changed, otherwise this function returns a*/
   /* negative value which signifies an error condition.                */
   /* * NOTE * The Inquiry Mode dictates how the local device will      */
   /*          actually perform inquiries (and how the results will be  */
   /*          returned).  The following table shows supported modes and*/
   /*          the corresponding Inquiry Result Event for that mode.    */
   /*                                                                   */
   /*             -------------------------------------------------     */
   /*             -    Mode    -     Inquiry Result Event         -     */
   /*             -------------------------------------------------     */
   /*             - imStandard - etInquiry_Entry_Result           -     */
   /*             - imRSSI     - etInquiry_With_RSSI_Entry_Result -     */
   /*             - imExtended - etExtended_Inquiry_Entry_Result  -     */
   /*             -------------------------------------------------     */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Inquiry_Mode(unsigned int BluetoothStackID, GAP_Inquiry_Mode_t GAP_Inquiry_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Inquiry_Mode_t)(unsigned int BluetoothStackID, GAP_Inquiry_Mode_t GAP_Inquiry_Mode);
#endif

   /* The following function is provided to allow a means to query the  */
   /* Current Inquiry Mode Settings for the Bluetooth Device that is    */
   /* specified by the Bluetooth Protocol Stack that is associated with */
   /* the specified Bluetooth Stack ID.  The second parameter to this   */
   /* function is a pointer to a variable that will receive the current */
   /* Inquiry Mode of the Bluetooth Device.  The second parameter must  */
   /* be valid (i.e.  NON-NULL) and upon successful completion of this  */
   /* function will contain the current Inquiry Mode of the Local       */
   /* Bluetooth Device.  This function will return zero on success, or a*/
   /* negative return error code if there was an error.  If this        */
   /* function returns success, then the GAP Inquiry Mode will contain  */
   /* the current Pairability Mode.                                     */
   /* * NOTE * The Inquiry Mode dictates how the local device will      */
   /*          actually perform inquiries (and how the results will be  */
   /*          returned).  The following table shows supported modes and*/
   /*          the corresponding Inquiry Result Event for that mode.    */
   /*                                                                   */
   /*             -------------------------------------------------     */
   /*             -    Mode    -     Inquiry Result Event         -     */
   /*             -------------------------------------------------     */
   /*             - imStandard - etInquiry_Entry_Result           -     */
   /*             - imRSSI     - etInquiry_With_RSSI_Entry_Result -     */
   /*             - imExtended - etExtended_Inquiry_Entry_Result  -     */
   /*             -------------------------------------------------     */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Inquiry_Mode(unsigned int BluetoothStackID, GAP_Inquiry_Mode_t *GAP_Inquiry_Mode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Inquiry_Mode_t)(unsigned int BluetoothStackID, GAP_Inquiry_Mode_t *GAP_Inquiry_Mode);
#endif

   /* The following function is provided to allow a mechanism to Query  */
   /* the User Friendly Bluetooth Device Name of the specified          */
   /* Bluetooth Device.  This function accepts as input the Bluetooth   */
   /* Protocol Stack of the Bluetooth Device that is to issue the       */
   /* Name Request, the Bluetooth Board Address of the Remote Bluetooth */
   /* Device to query, and the GAP Event Callback Information that is   */
   /* to be used when the Remote Device Name has been determined.  This */
   /* function returns zero if successful, or a negative return error   */
   /* code if the Remote Name Request was unable to be submitted.  If   */
   /* this function returns success, then the caller will be notified   */
   /* via the specified Callback when the specified information has     */
   /* been determined (or if there was an error).                       */
   /* * NOTE * This function cannot be used to determine the User       */
   /*          Friendly name of the Local Bluetooth Device.  The        */
   /*          GAP_Query_Local_Name() function should be used for this  */
   /*          purpose.  This function will fail if the Local Device's  */
   /*          Bluetooth Address is specified.                          */
   /* * NOTE * Because this function is asynchronous in nature          */
   /*          (specifying a remote BD_ADDR), this function will notify */
   /*          the caller of the result via the installed Callback.  The*/
   /*          caller is free to cancel the Remote Name Request at any  */
   /*          time by issuing the GAP_Cancel_Query_Remote_Name()       */
   /*          function and specifying the BD_ADDR of the Bluetooth     */
   /*          Device that was specified in this call.  It should be    */
   /*          noted that when the Callback is cancelled, the Callback  */
   /*          is the ONLY thing that is cancelled (i.e. the GAP module */
   /*          still performs the Remote Name Inquiry, it's just that   */
   /*          NO Callback is issued).                                  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Remote_Device_Name(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Remote_Device_Name_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to cancel the future calling   */
   /* of a Remote Name Result Event Callback that was installed via a   */
   /* successful call to the GAP_Query_Remote_Device_Name() function.   */
   /* This function DOES NOT cancel the Querying of the Remote Device's */
   /* Name, it ONLY cancels the Callback Notification.  This function   */
   /* accepts as input the Bluetooth Protocol Stack ID of the Bluetooth */
   /* Device that the GAP_Query_Remote_Device_Name() function was       */
   /* previously issued on, and the Board Address of the Bluetooth      */
   /* Device that the previous call was called with.  The BD_ADDR       */
   /* parameter MUST be valid, and cannot be the BD_ADDR or the Local   */
   /* Bluetooth Device because the Local Device Name Request does not   */
   /* use the Callback mechanism.  This function returns zero if        */
   /* successful, or a negative return error code if the function was   */
   /* unsuccessful.                                                     */
BTPSAPI_DECLARATION int BTPSAPI GAP_Cancel_Query_Remote_Device_Name(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Cancel_Query_Remote_Device_Name_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is provided to allow a mechanism to Query  */
   /* the features of the specified Bluetooth Device.  This function    */
   /* accepts as input the Bluetooth Protocol Stack ID of the Bluetooth */
   /* Device that is to issue the Feature Request, the Remote Bluetooth */
   /* Device Address that references the Remote Bluetooth Device, and   */
   /* the GAP Event Callback Information that is to be used when the    */
   /* Remote Feature Information has been determined.  This function    */
   /* returns zero if successful, or a negative return error code if the*/
   /* Remote Name Request was unable to be submitted.  If this function */
   /* returns success, then the caller will be notified via the         */
   /* specified callback when the requested information has been        */
   /* determined (or if there was an error).                            */
   /* * NOTE * Because this function is asynchronous in nature , this   */
   /*          function will notify the caller of the result via the    */
   /*          installed Callback.                                      */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Remote_Features(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Remote_Features_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a mechanism to Query  */
   /* the Version information of the specified Bluetooth Device.  This  */
   /* function accepts as input the Bluetooth Protocol Stack ID of the  */
   /* Bluetooth Device that is to issue the Version Request, the Remote */
   /* Bluetooth Device Address that references the Remote Bluetooth     */
   /* Device, and the GAP Event Callback Information that is to be used */
   /* when the Remote Version Information has been determined.  This    */
   /* function returns zero if successful, or a negative return error   */
   /* code if the Remote Version Request was unable to be submitted.  If*/
   /* this function returns success, then the caller will be notified   */
   /* via the specified callback when the requested information has been*/
   /* determined (or if there was an error).                            */
   /* * NOTE * Because this function is asynchronous in nature , this   */
   /*          function will notify the caller of the result via the    */
   /*          installed Callback.                                      */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Remote_Version_Information(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Remote_Version_Information_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a means to Initiate   */
   /* a Bonding Procedure.  This function can perform both General      */
   /* and Dedicated Bonding based upon the type of Bonding requested.   */
   /* This function accepts as input, the Bluetooth Protocol Stack ID   */
   /* of the Local Bluetooth Device that is perform the Bonding, the    */
   /* Remote Bluetooth Address of the Device to Bond with, the type of  */
   /* bonding to perform, and the GAP Event Callback Information that   */
   /* will be used to handle Authentication Events that will follow if  */
   /* this function is successful.  This function returns zero if       */
   /* successful, or a negative return error code if unsuccessful.  If  */
   /* this function is successful then all further information will     */
   /* be returned through the Registered GAP Event Callback.  It should */
   /* be noted that if this function returns success that it does NOT   */
   /* mean that the Remote Device has successfully Bonded with the      */
   /* Local Device, it ONLY means that the Remote Device Bonding Process*/
   /* has been started.  This function will only succeed if a Physical  */
   /* Connection to the specified Remote Bluetooth Device does NOT      */
   /* already exist.  This function will connect to the Bluetooth       */
   /* Device and begin the Bonding Process.  If General Bonding is      */
   /* specified then the Link is maintained, and will NOT be            */
   /* terminated until the GAP_End_Bonding() function has been called.  */
   /* This will allow for any higher level initialization that is needed*/
   /* on the same physical link.  If Dedicated Bonding is performed,    */
   /* then the Link is terminated automatically when the Authentication */
   /* Process has completed.                                            */
   /* * NOTE * Due to the asynchronous nature of this process, the GAP  */
   /*          Event Callback that is specified will inform the caller  */
   /*          of any Events and/or Data that is part of the            */
   /*          Authentication Process.  The GAP_Cancel_Bonding()        */
   /*          function can be called at any time to end the Bonding    */
   /*          Process and terminate the link (regardless of which      */
   /*          Bonding method is being performed).                      */
   /* * NOTE * When using General Bonding, if an L2CAP Connection is    */
   /*          established over the Bluetooth Link that was initiated   */
   /*          with this function, the Bluetooth Protocol Stack MAY or  */
   /*          MAY NOT terminate the Physical Link when (and if) an     */
   /*          L2CAP Disconnect Request (or Response) is issued.  If    */
   /*          this occurs, then calling GAP_End_Bonding() will have    */
   /*          no effect (the GAP_End_Bonding() function will return    */
   /*          an error code in this case).                             */
BTPSAPI_DECLARATION int BTPSAPI GAP_Initiate_Bonding(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Bonding_Type_t GAP_Bonding_Type, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Initiate_Bonding_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Bonding_Type_t GAP_Bonding_Type, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow a means to Cancel a   */
   /* Bonding process that was started previously via a successful call */
   /* to the GAP_Initiate_Bonding() function.  This function accepts as */
   /* input the Bluetooth Protocol Stack ID of the Local Bluetooth      */
   /* Device that the Bonding Process was initiated on and the Bluetooth*/
   /* Board Address of the Remote Bluetooth Device that the bonding     */
   /* procedure was initiated with.  This function returns zero if the  */
   /* Bonding Procedure was successfully terminated, or a negative      */
   /* return value if there was an error.  This function terminates the */
   /* Connection and NO further GAP Event Callbacks will be issued      */
   /* after this function has completed (if successful).                */
BTPSAPI_DECLARATION int BTPSAPI GAP_Cancel_Bonding(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Cancel_Bonding_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is provided to allow a means to terminate  */
   /* a Link that was established via a call to the                     */
   /* GAP_Initiate_Bonding() function (that specified General Bonding   */
   /* as the Bonding Type to perform).  This function has NO effect if  */
   /* the Bonding Procedure was initiated using Dedicated Bonding.      */
   /* This function accepts as input, the Bluetooth Protocol Stack ID   */
   /* of the Local Bluetooth Device that the General Bonding Process    */
   /* was previously initiated on and the Bluetooth Board Address of    */
   /* the Remote Bluetooth Device that was specified to be Bonded with. */
   /* This function returns zero if successful, or a negative return    */
   /* error code if there was an error.  This function terminates the   */
   /* Connection that was established and it guarantees that NO GAP     */
   /* Event Callbacks will be issued to the GAP Event Callback that was */
   /* specified in the original GAP_Initiate_Bonding() function call    */
   /* (if this function returns success).                               */
BTPSAPI_DECLARATION int BTPSAPI GAP_End_Bonding(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_End_Bonding_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is responsible for Querying (and reporting)*/
   /* the Board Address of the Local Bluetooth Device that is specified */
   /* by the Bluetooth Protocol Stack specified by the Bluetooth Stack  */
   /* ID (first parameter). The second parameter is a pointer to a      */
   /* Buffer that is to receive the Board Address of the Local Device.  */
   /* If this function is successful, this function returns zero, and   */
   /* the buffer that BD_ADDR points to will be filled with the Board   */
   /* Address read from the Local Device.  If this function returns a   */
   /* negative value, then the BD_ADDR of the Local Device was NOT able */
   /* to be queried (error condition).                                  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Local_BD_ADDR(unsigned int BluetoothStackID, BD_ADDR_t *BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Local_BD_ADDR_t)(unsigned int BluetoothStackID, BD_ADDR_t *BD_ADDR);
#endif

   /* The following function is provided to allow the changing of the   */
   /* Class of Device of the Local Device specified by the Bluetooth    */
   /* Protocol Stack that is specified by the Bluetooth Stack ID        */
   /* parameter.  The Class of Device Parameter represents the Class of */
   /* Device value that is to be written to the Local Device.  This     */
   /* function will return zero if the Class of Device was successfully */
   /* changed, or a negative return error code if there was an error    */
   /* condition.                                                        */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Class_Of_Device(unsigned int BluetoothStackID, Class_of_Device_t Class_of_Device);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Class_Of_Device_t)(unsigned int BluetoothStackID, Class_of_Device_t Class_of_Device);
#endif

   /* The following function is responsible for Querying (and reporting)*/
   /* the Class of Device of the Local Bluetooth Device that is         */
   /* specified by the Bluetooth Protocol Stack specified by the        */
   /* Bluetooth Stack ID (first parameter). The second parameter is a   */
   /* pointer to a Buffer that is to receive the Class of Device of the */
   /* Local Device.  If this function is successful, this function      */
   /* returns zero, and the buffer that Class_Of_Device points to will  */
   /* be filled with the Class of Device read from the Local Device.    */
   /* If this function returns a negative value, then the Class of      */
   /* Device of the Local Device was NOT able to be queried (error      */
   /* condition).                                                       */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Class_Of_Device(unsigned int BluetoothStackID, Class_of_Device_t *Class_of_Device);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Class_Of_Device_t)(unsigned int BluetoothStackID, Class_of_Device_t *Class_of_Device);
#endif

   /* The following function is provided to allow the changing of the   */
   /* Device Name of the Local Bluetooth Device specified by the        */
   /* Bluetooth Protocol Stack that is specified by the Bluetooth Stack */
   /* ID parameter.  The Name parameter must be a pointer to a NULL     */
   /* terminated ASCII String of at most MAX_NAME_LENGTH (not counting  */
   /* the trailing NULL terminator).  This function will return zero if */
   /* the Name was successfully changed, or a negative return error code*/
   /* if there was an error condition.                                  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Set_Local_Device_Name(unsigned int BluetoothStackID, char *Name);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Set_Local_Device_Name_t)(unsigned int BluetoothStackID, char *Name);
#endif

   /* The following function is responsible for Querying (and reporting)*/
   /* the User Friendly Name of the Local Bluetooth Device that is      */
   /* specified by the Bluetooth Protocol Stack specified by the        */
   /* Bluetooth Stack ID (first parameter). The second and third        */
   /* parameters to this function specify the buffer and buffer length  */
   /* of the buffer that is to receive the Local Name.  The NameBuffer  */
   /* Length should be at least (MAX_NAME_LENGTH+1) to hold the         */
   /* Maximum allowable Name (plus a single character to hold the NULL  */
   /* terminator).  If this function is successful, this function       */
   /* returns zero and the buffer that NameBuffer points to will be     */
   /* filled with a NULL terminated ASCII representation of the Local   */
   /* Device Name.  If this function returns a negative value, then the */
   /* Local Device Name was NOT able to be queried (error condition).   */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Local_Device_Name(unsigned int BluetoothStackID, unsigned int NameBufferLength, char *NameBuffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Local_Device_Name_t)(unsigned int BluetoothStackID, unsigned int NameBufferLength, char *NameBuffer);
#endif

   /* The following function is provided to allow a means to disconnect */
   /* an established Connection Link (ACL Connection) from the Protocol */
   /* Stack.  This function accepts as it's input parameters the        */
   /* Bluetooth Protocol Stack ID of the Bluetooth Protocol Stack that  */
   /* the connection exists for and the Bluetooth Board Address of the  */
   /* Remote Bluetooth Device which is connected.  This function returns*/
   /* zero if successful, or a negative return error code if there was  */
   /* an error disconnecting the specified link.                        */
   /* * NOTE * This function should be used sparingly as it will NOT    */
   /*          send protocol specific disconnections (i.e. it will NOT  */
   /*          send an RFCOMM Disconnect or a L2CAP Disconnect Request).*/
   /*          This function is a very low-level function in that it    */
   /*          simply kills the ACL Link that has been established with */
   /*          the specified Bluetooth Device.                          */
BTPSAPI_DECLARATION int BTPSAPI GAP_Disconnect_Link(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Disconnect_Link_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

   /* The following function is provided to allow a means to query the  */
   /* ACL Connection Handle of a connection to a remote Bluetooth       */
   /* Device.  This function accepts as it's input parameters the       */
   /* Bluetooth Protocol Stack ID of the Bluetooth Protocol Stack that  */
   /* the connection exists, the Bluetooth Board Address of the Remote  */
   /* Bluetooth Device which is connected, and a pointer to a variable  */
   /* that will receive the Connection Handle for the connection to the */
   /* specified Bluetooth Board Address.  This function will return zero*/
   /* on success, or a negative return error code if there was an error.*/
   /* If this function returns success, then the Connection Handle      */
   /* variable will contain the current ACL Connection Handle for the   */
   /* connection to the specified Bluetooth Board Address.              */
   /* * NOTE * If this function returns with an error, negative value,  */
   /*          the value returned in the Connection_Handle variable     */
   /*          should be considered invalid.                            */
   /* * NOTE * If this function returns BTPS_ERROR_DEVICE_NOT_CONNECTED */
   /*          a connection to the specified Bluetooth Board Address    */
   /*          does not exist.                                          */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Connection_Handle(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Word_t *Connection_Handle);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Connection_Handle_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Word_t *Connection_Handle);
#endif

   /* The following function is provided to for Local devices that      */
   /* support Out of Band, OOB, pairing using a technology such as NFC, */
   /* Near Field communications. It is used to obtain the Simple        */
   /* Pairing Hash C and the Simple Pairing Randomizer R which are      */
   /* intended to be transferred to a remote device using OOB. A new    */
   /* value for C and R are r                                           */
   /* * NOTE * A new value for C and R are created each time this call  */
   /*          is made. Each OOB transfer will have unique C and R      */
   /*          values so after each OOB transfer this function should   */
   /*          be called to obtain a new set for the next OOB transfer. */
   /* * NOTE * These values are not kept on a reset or power off in     */
   /*          which case a call to this should be invoked during time  */
   /*          time of initialization.                                  */
BTPSAPI_DECLARATION int BTPSAPI GAP_Query_Local_Out_Of_Band_Data(unsigned int BluetoothStackID, GAP_Out_Of_Band_Data_t *OutOfBandData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Query_Local_Out_Of_Band_Data_t)(unsigned int BluetoothStackID, GAP_Out_Of_Band_Data_t *OutOfBandData);
#endif

   /* The following function is provided to allow the host to cause the */
   /* Controller to refresh the encryption by pausing then resuming.    */
   /* A BD_ADDR type is passed and from that a 'valid' connection       */
   /* handle is determined, otherwise an error shall be returned.       */
   /* * NOTE * Because this function is asynchronous in nature, this    */
   /*          function will notify the caller of the completion of a   */
   /*          refresh via the installed Callback. This operation       */
   /*          cannot be cancelled at any time other than a disconnect. */
BTPSAPI_DECLARATION int BTPSAPI GAP_Refresh_Encryption_Key(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Refresh_Encryption_Key_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_Event_Callback_t GAP_Event_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is provided to allow the local host to read*/
   /* the Extended Inquiry Response Information currently stored in the */
   /* controller.  This is the data that the controller will return when*/
   /* it returns an extended inquiry response to a remote device.  This */
   /* function will return zero if successful, or a negative return     */
   /* error code if there was an error condition.  If this function     */
   /* returns success, then the Extended_Inquiry_Response_Data member   */
   /* will be filled in with the correct data.                          */
   /* * NOTE * The GAP_Parse_Extended_Inquiry_Response_Data() function  */
   /*          can be used to parse the Extended Inquiry Response Data  */
   /*          for easy parsing (if required).                          */
BTPSAPI_DECLARATION int BTPSAPI GAP_Read_Extended_Inquiry_Information(unsigned int BluetoothStackID, Byte_t *FEC_Required, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Read_Extended_Inquiry_Information_t)(unsigned int BluetoothStackID, Byte_t *FEC_Required, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);
#endif

   /* The following function is provided to allow the local host to     */
   /* write the extended inquiry information to be stored in the        */
   /* controller.  This is the data that the controller will return when*/
   /* it returns an extended inquiry response to a remote device.  This */
   /* function will return zero if successful, or a negative return     */
   /* error code if there was an error condition.                       */
BTPSAPI_DECLARATION int BTPSAPI GAP_Write_Extended_Inquiry_Information(unsigned int BluetoothStackID, Byte_t FEC_Required, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Write_Extended_Inquiry_Information_t)(unsigned int BluetoothStackID, Byte_t FEC_Required, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);
#endif

   /* The following function is provided to allow a simple mechanism to */
   /* convert a GAP_Extended_Inquiry_Response_Data_t to the raw         */
   /* Extended_Inquiry_Response_Data_t.  This second parameter *MUST*   */
   /* point to the maximum sized Extended Inquiry Response Buffer size  */
   /* (EXTENDED_INQUIRY_RESPONSE_DATA_SIZE).  This function will return */
   /* the number of successfully converted items (zero or more), or a   */
   /* negative error code if there was an error.                        */
   /* * NOTE * This function will populate the entire                   */
   /*          Extended_Inquiry_Response_Data_t buffer (all             */
   /*          EXTENDED_INQUIRY_RESPONSE_DATA_SIZE bytes).  If the      */
   /*          specified information is smaller than the full           */
   /*          Extended Inquiry Response Data size, the resulting buffer*/
   /*          will be padded with zeros.                               */
BTPSAPI_DECLARATION int BTPSAPI GAP_Convert_Extended_Inquiry_Response_Data(GAP_Extended_Inquiry_Response_Data_t *GAP_Extended_Inquiry_Response_Data, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Convert_Extended_Inquiry_Response_Data_t)(GAP_Extended_Inquiry_Response_Data_t *GAP_Extended_Inquiry_Response_Data, Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data);
#endif

   /* The following function is a utility function that exists to parse */
   /* the specified Extended_Inquiry_Response_Data_t information into   */
   /* a GAP_Extended_Inquiry_Response_Data_t structure (for ease of     */
   /* parsing).  This function accepts as the first parameter the       */
   /* Extended_Inquiry_Response_Data_t to parse, followed by a pointer  */
   /* to a GAP_Extended_Inquiry_Response_Data_t that will receive the   */
   /* Parsed data.  The final parameter, if specified, *MUST* specify   */
   /* the maximum number of entries that can be parsed, as well as the  */
   /* actual Entry array to parse the entries into (on input).          */
   /* * NOTE * If this function is called with a NULL passed as the     */
   /*          final two parameters, then, this function will simply    */
   /*          calculate the number of Extended Inquiry Data Information*/
   /*          Entries that will be required to hold the parsed         */
   /*          information.  If the final parameter is NOT NULL then it */
   /*          *MUST* contain the maximum number of entries that can    */
   /*          be supported (specified via the Number_Data_Entries      */
   /*          member) and the Data_Entries member must point to memory */
   /*          that contains (at least) that many members).             */
   /* * NOTE * This function will return                                */
   /*                                                                   */
   /*             BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE                  */
   /*                                                                   */
   /*          if there was not enough Data Entries specified (via the  */
   /*          Number_Data_Entries member) to satisfy the parsing of    */
   /*          the actual Extended Inquiry Response Data.               */
BTPSAPI_DECLARATION int BTPSAPI GAP_Parse_Extended_Inquiry_Response_Data(Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data, GAP_Extended_Inquiry_Response_Data_t *GAP_Extended_Inquiry_Response_Data);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_GAP_Parse_Extended_Inquiry_Response_Data_t)(Extended_Inquiry_Response_Data_t *Extended_Inquiry_Response_Data, GAP_Extended_Inquiry_Response_Data_t *GAP_Extended_Inquiry_Response_Data);
#endif

#endif
