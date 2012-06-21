/*****< sdpapi.h >*************************************************************/
/*      Copyright 2000 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  SDPAPI - Stonestreet One Bluetooth Stack SDP API Type Definitions,        */
/*           Constants, and Prototypes.                                       */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   09/11/00  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __SDPAPIH__
#define __SDPAPIH__

#include "BTPSKRNL.h"           /* BTPS Kernel Prototypes/Constants.          */

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "SDPTypes.h"           /* Bluetooth SDP Type Definitions/Constants.  */

#include "BTPSCFG.h"            /* BTPS Configuration Constants.              */

   /* The following enumerated type represents the different Connection */
   /* Modes that are supported by SDP.  These constants are used with   */
   /* the SDP_Set_Disconnect_Mode() function.                           */
typedef enum
{
   dmAutomatic,
   dmManual
} SDP_Disconnect_Mode_t;

   /* The following constant specifies the default SDP Disconnect Mode  */
   /* that is used by SDP by default.                                   */
#define SDP_DEFAULT_SERVER_DISCONNECT_MODE                           (BTPS_CONFIGURATION_SDP_DEFAULT_DISCONNECT_MODE)

   /* The following enumerated type represents all the allowable SDP    */
   /* Data Element Types that can be used with the SDP API.             */
typedef enum
{
   deNIL,
   deNULL,
   deUnsignedInteger1Byte,
   deUnsignedInteger2Bytes,
   deUnsignedInteger4Bytes,
   deUnsignedInteger8Bytes,
   deUnsignedInteger16Bytes,
   deSignedInteger1Byte,
   deSignedInteger2Bytes,
   deSignedInteger4Bytes,
   deSignedInteger8Bytes,
   deSignedInteger16Bytes,
   deTextString,
   deBoolean,
   deURL,
   deUUID_16,
   deUUID_32,
   deUUID_128,
   deSequence,
   deAlternative
} SDP_Data_Element_Type_t;

   /* The following data type represents a special SDP Data Type.  This */
   /* structure ONLY holds the UUID Data Types.  This structure is      */
   /* provided so that API calls that only deal with UUID's can use     */
   /* this data type instead of the more generic SDP_Data_Element_Type_t*/
   /* Data Type.  This will aid in code readability and also aid in     */
   /* making the code that only processes UUID's more simple.           */
typedef struct _tagSDP_UUID_Entry_t
{
   SDP_Data_Element_Type_t SDP_Data_Element_Type;
   union
   {
      UUID_16_t  UUID_16;
      UUID_32_t  UUID_32;
      UUID_128_t UUID_128;
   } UUID_Value;
} SDP_UUID_Entry_t;

#define SDP_UUID_ENTRY_SIZE                             (sizeof(SDP_UUID_Entry_t))

   /* The following data type represents a special SDP Data Type.  This */
   /* structure ONLY holds the Attribute ID Information.  This          */
   /* structure is provided so that API calls that only deal with       */
   /* Attribute ID Lists can use this data type instead of the more     */
   /* generic SDP_Data_Element_Type_t Data Type.  This will aid in code */
   /* readability and also aid in making the code that only processes   */
   /* Attribute ID Lists more simple.                                   */
   /* The BOOLEAN Flag specifies whether or not the Attribute ID List   */
   /* specifies a Range or a single Attribute ID.  If this flag is      */
   /* TRUE, then the End_Attribute_ID is valid and is used.  The        */
   /* End_Attribute_ID value MUST be greater than the Start_Attribute_ID*/
   /* value of the entry is considered invalid (if the Attribute_Range  */
   /* flag is TRUE).  If the Attribute_Range member is FALSE, then the  */
   /* Start_Attribute_ID member is the only member that is used.        */
typedef struct _tagSDP_Attribute_ID_List_Entry_t
{
   Boolean_t Attribute_Range;
   Word_t    Start_Attribute_ID;
   Word_t    End_Attribute_ID;
} SDP_Attribute_ID_List_Entry_t;

#define SDP_ATTRIBUTE_ID_LIST_ENTRY_SIZE                (sizeof(SDP_Attribute_ID_List_Entry_t))

   /* The following Data Structure represents a structure that will hold*/
   /* an individual SDP Data Element.  The SDP_Data_Element_Type field  */
   /* holds the SDP Data Element Type, the Length field holds the number*/
   /* of Bytes that the Actual Data Element Value occupies (this value  */
   /* represents the buffer size that the pointer member of the union   */
   /* points to), and finally, the union defines the actual Data Type   */
   /* value.  It should be noted that the Text Field and the URL        */
   /* members are pointers to data because the data itself is variable  */
   /* in length.                                                        */
   /* * NOTE * The following structure also supports the SDP Data Types */
   /*          of Sequences and Alternatives.  This is treated as any   */
   /*          of the other Data Element Types, the                     */
   /*          SDP_Data_Element_Length field denotes the Number of the  */
   /*          Data Elements that the SDP_Data_Element_Sequence OR the  */
   /*          SDP_Data_Element_Alternative Member points to.           */
typedef struct _tagSDP_Data_Element_t
{
   SDP_Data_Element_Type_t           SDP_Data_Element_Type;
   DWord_t                           SDP_Data_Element_Length;
   union
   {
      Byte_t                         UnsignedInteger1Byte;
      Word_t                         UnsignedInteger2Bytes;
      DWord_t                        UnsignedInteger4Bytes;
      Byte_t                         UnsignedInteger8Bytes[8];
      Byte_t                         UnsignedInteger16Bytes[16];
      SByte_t                        SignedInteger1Byte;
      SWord_t                        SignedInteger2Bytes;
      SDWord_t                       SignedInteger4Bytes;
      Byte_t                         SignedInteger8Bytes[8];
      Byte_t                         SignedInteger16Bytes[16];
      Byte_t                         Boolean;
      UUID_16_t                      UUID_16;
      UUID_32_t                      UUID_32;
      UUID_128_t                     UUID_128;
      Byte_t                        *TextString;
      Byte_t                        *URL;
      struct _tagSDP_Data_Element_t *SDP_Data_Element_Sequence;
      struct _tagSDP_Data_Element_t *SDP_Data_Element_Alternative;
   } SDP_Data_Element;
} SDP_Data_Element_t;

#define SDP_DATA_ELEMENT_SIZE                           (sizeof(SDP_Data_Element_t))

   /* The following enumerated type represents all the allowable SDP    */
   /* Request Response Data Types that will be returned in the SDP      */
   /* Response Callback Function.                                       */
typedef enum
{
   rdTimeout,
   rdConnectionError,
   rdErrorResponse,
   rdServiceSearchResponse,
   rdServiceAttributeResponse,
   rdServiceSearchAttributeResponse
} SDP_Response_Data_Type_t;

   /* The following Data Structure represents the SDP Error Response    */
   /* Information that is returned by a Remote SDP Server when an       */
   /* invalid request has been received.  The Error_Info field is an    */
   /* optional field that may or may be present depending upon the      */
   /* Error Code value.  If there is NO Error Information, then the     */
   /* Error_Info_Length member will be set to zero and the Error_Info   */
   /* member pointer will be NULL.  If there is Error Information, then */
   /* the Error_Info member will be a non-NULL pointer to the Error     */
   /* Information, and the Error_Info_Length member will contain the    */
   /* Length of the Data (in Bytes) that the Error_Info Pointer points  */
   /* to.                                                               */
typedef struct _tagSDP_Error_Response_Data_t
{
   Word_t   Error_Code;
   Word_t   Error_Info_Length;
   Byte_t  *Error_Info;
} SDP_Error_Response_Data_t;

#define SDP_ERROR_RESPONSE_DATA_SIZE                    (sizeof(SDP_Error_Response_Data_t))

   /* The following Data Structure represents the SDP Service Search    */
   /* Response Data that is returned by the Remote SDP Server when a    */
   /* SDP Service Search Request is submitted.  If there are Service    */
   /* Records then the Total_Service_Record_Count member will contain   */
   /* the Number Service Record Handles that the Service_Record_List    */
   /* member points to.  If there are NO Service Record Handles, then   */
   /* the Total_Service_Record_Count member will be zero, and the       */
   /* Service_Record_List member will set to NULL.                      */
typedef struct _tagSDP_Service_Search_Response_Data_t
{
   Word_t   Total_Service_Record_Count;
   DWord_t *Service_Record_List;
} SDP_Service_Search_Response_Data_t;

#define SDP_SERVICE_SEARCH_RESPONSE_DATA_SIZE           (sizeof(SDP_Service_Search_Response_Data_t))

   /* The following Data Structure represents a Single SDP Service      */
   /* Attribute.  This Attribute consists of an Attribute ID and the    */
   /* Attribute Data which is a SDP Data Element (Note the SDP Data     */
   /* Element could contain a list of SDP Data Elements).               */
typedef struct _tagSDP_Attribute_Value_Data_t
{
   Word_t              Attribute_ID;
   SDP_Data_Element_t *SDP_Data_Element;
} SDP_Service_Attribute_Value_Data_t;

#define SDP_SERVICE_ATTRIBUTE_VALUE_DATA_SIZE           (sizeof(SDP_Service_Attribute_Value_Data_t))

   /* The following Data Structure represents an Attribute List that    */
   /* the Remote SDP Server returns when a SDP Service Attribute Request*/
   /* is processed.  If there are Attributes, then the                  */
   /* Number_Attribute_Values member will contain a non-zero value, and */
   /* the SDP_Service_Attribute_Value_Data will point to an array of    */
   /* Service Attributes (Attribute ID/Attribute Data).  If the         */
   /* Number_Attribute_Values member is zero, then there are NO         */
   /* Attributes present in the list, and the                           */
   /* SDP_Service_Attribute_Value_Data member will be NULL.             */
typedef struct _tagSDP_Attribute_Response_Data_t
{
   Word_t                              Number_Attribute_Values;
   SDP_Service_Attribute_Value_Data_t *SDP_Service_Attribute_Value_Data;
} SDP_Service_Attribute_Response_Data_t;

#define SDP_SERVICE_ATTRIBUTE_RESPONSE_DATA_SIZE        (sizeof(SDP_Service_Attribute_Response_Data_t))

   /* The following Data Structure represents a Service Record Attribute*/
   /* List that the Remote SDP Server returns when a SDP Service Search */
   /* Attribute Request is processed.  If there were Service Records    */
   /* that matched the requested Search Pattern, then the               */
   /* Number_Service_Records member will contain a non-zero value, and  */
   /* the SDP_Service_Attribute_Response member will point to an array  */
   /* of SDP_Service_Attribute_Response_t data structures.  Each element*/
   /* of this array will specify the Attribute(s) for an individual     */
   /* Service Record .  If the Number_Service_Records member is zero,   */
   /* then there are NO Service Records (and thus NO Attributes)        */
   /* present in the list, and the SDP_Service_Attribute_Response_Data  */
   /* member will be NULL.                                              */
typedef struct _tagSDP_Search_Attribute_Response_Data_t
{
   Word_t                                 Number_Service_Records;
   SDP_Service_Attribute_Response_Data_t *SDP_Service_Attribute_Response_Data;
} SDP_Service_Search_Attribute_Response_Data_t;

#define SDP_SERVICE_SEARCH_ATTRIBUTE_RESPONSE_DATA_SIZE (sizeof(SDP_Service_Search_Attribute_Response_Data_t))

   /* The following Data Structure represents the Data that is          */
   /* returned in the SDP Response Callback.  This Data Structure       */
   /* is a container for ALL possible Data that could be returned in    */
   /* a SDP Response Callback.  Note that the type of Data returned     */
   /* depends on the SDP Request that the callback was installed for.   */
   /* Also note that for the rdTimeout and rdConnectionError            */
   /* SDP_Response_Data_Types there is NO further Data available for    */
   /* these Errors.                                                     */
typedef struct _tagSDP_Response_Data_t
{
   SDP_Response_Data_Type_t SDP_Response_Data_Type;
   union
   {
      SDP_Error_Response_Data_t                    SDP_Error_Response_Data;
      SDP_Service_Search_Response_Data_t           SDP_Service_Search_Response_Data;
      SDP_Service_Attribute_Response_Data_t        SDP_Service_Attribute_Response_Data;
      SDP_Service_Search_Attribute_Response_Data_t SDP_Service_Search_Attribute_Response_Data;
   } SDP_Response_Data;
} SDP_Response_Data_t;

#define SDP_RESPONSE_DATA_SIZE                          (sizeof(SDP_Response_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* a SDP Request Callback.  This function will be called whenever    */
   /* a SDP Request Response returns withith the Bluetooth Protocol     */
   /* Stack that is specified with the specified Bluetooth Stack ID.    */
   /* This function passes to the caller the Bluetooth Stack ID, the    */
   /* SDP Request ID that was assigned to the SDP Service Request, the  */
   /* SDP Response Data associated with the SDP Request Response that   */
   /* occurred, and the SDP Callback Parameter that was specified when  */
   /* this Callback was installed.  The caller is free to use the       */
   /* contents of the SDP Request Response Data ONLY in the context of  */
   /* this callback.  If the caller requires the Data for a longer      */
   /* period of time, then the callback function MUST copy the data     */
   /* into another Data Buffer(s).  This function is guaranteed NOT to  */
   /* be invoked more than once simultaneously for the specified        */
   /* installed callback (i.e. this function DOES NOT have to be        */
   /* reentrant).  The processing in this function should be as         */
   /* efficient as possible.  It should be noted that this function is  */
   /* called in the Thread Context of a Thread that the User does NOT   */
   /* own.  Therefore, processing in this function should be as         */
   /* possible.                                                         */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving other Bluetooth     */
   /*            Stack Events.  A Deadlock WILL occur because other     */
   /*            Callbacks might not be issued while this function is   */
   /*            currently outstanding.                                 */
typedef void (BTPSAPI *SDP_Response_Callback_t)(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter);

   /* The following function is responsible for adding an SDP Service   */
   /* Record to the SDP Database.  The first parameter to this function */
   /* is the Bluetooth Stack ID of the SDP Server to create the SDP     */
   /* Service Record on.  The second parameter is the number of         */
   /* UUID Entries that are present in the third parameter array.  The  */
   /* second parameter CANNOT be zero, and the the third parameter      */
   /* must contain at least as many entries as specified by the         */
   /* second parameter.  If this function is successful, this function  */
   /* will return a positive, non-zero, value which represents the      */
   /* SDP Server Record Handle of the Service Record that was created   */
   /* on the specified SDP Server.  This value can be used as the       */
   /* Service_Record_Handle parameter for the                           */
   /* SDP_Delete_Service_Record() function as well as the               */
   /* SDP_Add_Attribute() and the SDP_Delete_Attribute() function.      */
   /* This function returns a negative return value if the specified    */
   /* Service Record was unable to be added to the SDP Database.        */
   /* * NOTE * The UUID values that are specified in this Service       */
   /*          Record call represent the ServiceClassIDList Attributes  */
   /*          of the Service Record.  This parameter is required       */
   /*          because EVERY SDP Service Record MUST have this          */
   /*          Attribute.                                               */
BTPSAPI_DECLARATION long BTPSAPI SDP_Create_Service_Record(unsigned int BluetoothStackID, unsigned int NumberServiceClassUUID, SDP_UUID_Entry_t SDP_UUID_Entry[]);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef long (BTPSAPI *PFN_SDP_Create_Service_Record_t)(unsigned int BluetoothStackID, unsigned int NumberServiceClassUUID, SDP_UUID_Entry_t SDP_UUID_Entry[]);
#endif

   /* The following function is responsible for Deleting a SDP Service  */
   /* Record that was added with the SDP_Create_Service_Record()        */
   /* function.  This function accepts as input, the Bluetooth Stack ID */
   /* of the Bluetooth Protocol Stack that the SDP Server resides on    */
   /* and the SDP Service Record Handle to delete from the specified    */
   /* SDP Server. The second parameter to this function is obtained     */
   /* via a successful call to the SDP_Create_Service_Record()          */
   /* function.  This function deletes the specified SDP Service Record */
   /* and deletes ALL SDP Attributes that are associated with the       */
   /* specified Service Record.  This function returns zero if the      */
   /* specified Service Record was able to be deleted successfully, or  */
   /* a negative return error code if the Service Record was NOT able to*/
   /* be deleted successfully.  If this function completes successfully */
   /* the Service Record is NO longer valid on the specified SDP        */
   /* Server.                                                           */
BTPSAPI_DECLARATION int BTPSAPI SDP_Delete_Service_Record(unsigned int BluetoothStackID, DWord_t Service_Record_Handle);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Delete_Service_Record_t)(unsigned int BluetoothStackID, DWord_t Service_Record_Handle);
#endif

   /* The following function is responsible for Adding SDP Service      */
   /* Attributes to the specified SDP Service Record.  This function    */
   /* accepts as input the Bluetooth Stack ID of the Bluetooth Protocol */
   /* Stack that the SDP Server resides on and the SDP Service Record   */
   /* Handle to Add the specified Attribute.  The Service_Record_Handle */
   /* parameter must have been obtained via a successful call to the    */
   /* SDP_Create_Service_Record() function.  The third parameter        */
   /* specifies the Attribute Value that is to be associated with the   */
   /* specified Attribute. This value must contain a valid entry.  This */
   /* function returns zero if the specified Attribute was added        */
   /* successfully, or a negative return value if the specified         */
   /* attribute was unable to be added to the specified SDP Service     */
   /* Record.                                                           */
   /* * NOTE * The SDP_ATTRIBUTE_ID_SERVICE_RECORD_HANDLE and the       */
   /*          SDP_ATTRIBUTE_ID_SERVICE_CLASS_ID_LIST Attributes cannot */
   /*          be added since they were already added when the Service  */
   /*          Record was created with the SDP_Create_Service_Record()  */
   /*          function.                                                */
   /* * NOTE * Empty Lists can be added as an Attribute by specifying   */
   /*          that there are NO Data Elements present in the Data      */
   /*          Element Sequence or Data Element Alternative (Data       */
   /*          Element Length will be zero which signifies that the     */
   /*          list contains NO Data Elements).                         */
BTPSAPI_DECLARATION int BTPSAPI SDP_Add_Attribute(unsigned int BluetoothStackID, DWord_t Service_Record_Handle, Word_t Attribute_ID, SDP_Data_Element_t *SDP_Data_Element);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Add_Attribute_t)(unsigned int BluetoothStackID, DWord_t Service_Record_Handle, Word_t Attribute_ID, SDP_Data_Element_t *SDP_Data_Element);
#endif

   /* The following function is responsible for Deleting a SDP Attribute*/
   /* that was previously added via a successful call to the            */
   /* SDP_Add_Attribute() function.  This function accepts as input the */
   /* Bluetooth Stack ID of the Bluetooth Protocol Stack that the SDP   */
   /* Server resides on, the SDP Service Record Handle, and the         */
   /* Attribute ID of the Attribute to Delete.  This function returns   */
   /* zero if the Attribute was successfully deleted, or a negative     */
   /* return error code if the Attribute was NOT able to be deleted     */
   /* successfully.  If this function has completes successfully then   */
   /* the specifed Attribute no longer exists in the specified SDP      */
   /* Service Handle.                                                   */
   /* * NOTE * The SDP_ATTRIBUTE_ID_SERVICE_RECORD_HANDLE and the       */
   /*          SDP_ATTRIBUTE_ID_SERVICE_CLASS_ID_LIST Attributes cannot */
   /*          be deleted since they are required to exist for EVERY    */
   /*          SDP Service Record.                                      */
BTPSAPI_DECLARATION int BTPSAPI SDP_Delete_Attribute(unsigned int BluetoothStackID, DWord_t Service_Record_Handle, Word_t Attribute_ID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Delete_Attribute_t)(unsigned int BluetoothStackID, DWord_t Service_Record_Handle, Word_t Attribute_ID);
#endif

   /* The following function is responsible for issuing an SDP Service  */
   /* Search Request to the specified BD_ADDR.  This function will      */
   /* return the result of the Search Request in the SDP Response       */
   /* Callback that is specified in the calling of this function.  This */
   /* function accepts as input, the Bluetooth Stack ID of the Bluetooth*/
   /* Protocol Stack that the SDP Client resides on, the Bluetooth      */
   /* Board Address to remotely connect to (the Remote SDP Server will  */
   /* reside on this BD_ADDR), the Maximum Number of Service Records,   */
   /* the Number of Service UUID's that are to be searched for, the     */
   /* Service UUID's to actually search for, the SDP Response Callback  */
   /* Function, and the SDP Response Callback Function Callback         */
   /* Parameter.  This function will return a positive, non-zero,       */
   /* return code if successful, or a negative return error code if     */
   /* there was an error.  If this function is successful, the user can */
   /* call the SDP_Cancel_Service_Request() function to cancel a the    */
   /* SDP Service Search Request prematurely.  It should be noted that  */
   /* the Number of UUID Parameter must be at least one, and the        */
   /* Service UUID Parameter must point to a List of at least the       */
   /* Number of UUID's that have been specified.  Finally, the BD_ADDR  */
   /* Parameter and the SDP_Reponse_Callback Parameter MUST be valid or */
   /* the call to this function will be unsuccessful.                   */
BTPSAPI_DECLARATION int BTPSAPI SDP_Service_Search_Request(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Word_t MaximumServiceRecordCount, unsigned int NumberServiceUUID, SDP_UUID_Entry_t SDP_UUID_Entry[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Service_Search_Request_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, Word_t MaximumServiceRecordCount, unsigned int NumberServiceUUID, SDP_UUID_Entry_t SDP_UUID_Entry[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is responsible for issuing an SDP Service  */
   /* Attribute Request to the specified BD_ADDR.  This function will   */
   /* return the result of the Attribute Request in the SDP Response    */
   /* Callback that is specified in the calling of this function.  This */
   /* function accepts as input, the Bluetooth Stack ID of the Bluetooth*/
   /* Protocol Stack that the SDP Client resides on, the Bluetooth      */
   /* Board Address to remotely connect to (the Remote SDP Server will  */
   /* reside on this BD_ADDR), the Service Record Handle of the SDP     */
   /* Service Record to query, the Number of Entries in the Attribute   */
   /* List that are to be queried, the Attribute List to actually use   */
   /* in the Query, the SDP Response Callback Function, and the SDP     */
   /* Response Callback Function Callback Parameter.  This function will*/
   /* return a positive, non-zero, return code if successful, or a      */
   /* negative return error code if there was an error.  If this        */
   /* function is successful, the user can call the                     */
   /* SDP_Cancel_Service_Request() function to cancel a the SDP Service */
   /* Search Request prematurely.  It should be noted that the Number   */
   /* of Attributes that are contained in the Attribute List must be at */
   /* least one, and the Attribute ID List Parameter must point to a    */
   /* List of Attribute ID's that contains at least the Number of       */
   /* Attribute List Entries that have been specified.  Finally, the    */
   /* BD_ADDR Parameter and the SDP_Reponse_Callback Parameter MUST     */
   /* be valid or the call to this function will be unsuccessful.       */
BTPSAPI_DECLARATION int BTPSAPI SDP_Service_Attribute_Request(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, DWord_t ServiceRecordHandle, unsigned int NumberAttributeListElements, SDP_Attribute_ID_List_Entry_t AttributeIDList[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Service_Attribute_Request_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, DWord_t ServiceRecordHandle, unsigned int NumberAttributeListElements, SDP_Attribute_ID_List_Entry_t AttributeIDList[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);
#endif

   /* The following function is responsible for issuing an SDP Service  */
   /* Search Attribute Request to the specified BD_ADDR.  This function */
   /* will return the result of the Service Search Attribute Request in */
   /* the SDP Response Callback that is specified in the calling of this*/
   /* function.  This function accepts as input, the Bluetooth Stack ID */
   /* of the Bluetooth Protocol Stack that the SDP Client resides on,   */
   /* the Bluetooth Board Address to remotely connect to (the Remote    */
   /* SDP Server will reside on this BD_ADDR), the Number of Service    */
   /* UUID's that are to be searched for, the Servcee UUID's to actually*/
   /* search for, the Number of Entries in the Attribute List that are  */
   /* to be queried, the Attribute List to actually use in the Query,   */
   /* the SDP Response Callback Function, and the SDP Response Callback */
   /* Function Callback Parameter.  This function will return a         */
   /* positive, non-zero, return code if successful, or a negative      */
   /* return error code if there was an error.  If this function is     */
   /* successful, the user can call the SDP_Cancel_Service_Request()    */
   /* function to cancel a the SDP Service Search Request prematurely.  */
   /* It should be noted that the Number of UUID Parameter must be at   */
   /* least one, and the Service UUID Parameter must point to a List of */
   /* at least the Number of UUID's that have been specified.  Finally, */
   /* the BD_ADDR Parameter and the SDP_Reponse_Callback Parameter MUST */
   /* be valid or the call to this function will be unsuccessful.       */
   /* It should also be noted that the Number of Attributes that are    */
   /* in the Attribute List must be at least one, and the Attribute ID  */
   /* List Parameter must point to a List of Attribute ID's that        */
   /* contains at least the Number of Attribute List Entries that have  */
   /* been specified.  Finally, the BD_ADDR Parameter and the           */
   /* SDP_Reponse_Callback Parameter MUST be valid or the call to this  */
   /* function will be unsuccessful.                                    */
BTPSAPI_DECLARATION int BTPSAPI SDP_Service_Search_Attribute_Request(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, unsigned int NumberServiceUUID, SDP_UUID_Entry_t SDP_UUID_Entry[], unsigned int NumberAttributeListElements, SDP_Attribute_ID_List_Entry_t AttributeIDList[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Service_Search_Attribute_Request_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, unsigned int NumberServiceUUID, SDP_UUID_Entry_t SDP_UUID_Entry[], unsigned int NumberAttributeListElements, SDP_Attribute_ID_List_Entry_t AttributeIDList[], SDP_Response_Callback_t SDP_Response_Callback, unsigned long CallbackParameter);
#endif

   /* This function is responsible for terminating a currently          */
   /* executing SDP Service Request.  This function accepts as input the*/
   /* Bluetooth Protocol Stack ID of the Bluetooth Protocol Stack the   */
   /* SDP Service Request was issued on, and the SDP Service Request    */
   /* ID of the SDP Service Request that was issued.  The SDP Service   */
   /* Request ID is obtained via a successful call to one of the        */
   /* following functions:                                              */
   /*    - SDP_Service_Search_Request()                                 */
   /*    - SDP_Service_Attribute_Request()                              */
   /*    - SDP_Service_Search_Attribute_Request()                       */
   /* After this function is called, the callback that was installed    */
   /* for the specified SDP Service Request will NOT be called and the  */
   /* caller will NOT have access to the SDP Service Response           */
   /* Information for the SDP Service Request.                          */
BTPSAPI_DECLARATION void BTPSAPI SDP_Cancel_Service_Request(unsigned int BluetoothStackID, unsigned int ServiceRequestID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_SDP_Cancel_Service_Request_t)(unsigned int BluetoothStackID, unsigned int ServiceRequestID);
#endif

   /* This function is responsible for informing the SDP Module how it  */
   /* is to execute SDP Service Requests regarding the Connection       */
   /* Disconnection.  This function accepts as input the Bluetooth      */
   /* Protocol Stack ID of the Bluetooth Protocol Stack for which the   */
   /* SDP Server resides and the SDP Connection Mode that is to be      */
   /* set.  This function will return zero if the Connection Mode was   */
   /* successfully set, or a negative return error code if there was an */
   /* error.                                                            */
   /* * NOTE * If the caller specifies SDP Disconnect Mode dmManual     */
   /*          then the caller is responsible for disconnecting the     */
   /*          the SDP Connection (to the remote server) by calling     */
   /*          the SDP_Disconnect_Server() function.  If the SDP        */
   /*          Disconnect Mode dmAutomatic is chosen (default) then the */
   /*          Connection to the server is automatically terminated     */
   /*          when the SDP Transaction completes.                      */
   /* * NOTE * The SDP Connection Mode can ONLY be changed when there   */
   /*          are NO Client SDP Transactions outstanding.              */
BTPSAPI_DECLARATION int BTPSAPI SDP_Set_Disconnect_Mode(unsigned int BluetoothStackID, SDP_Disconnect_Mode_t SDPDisconnectMode);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Set_Disconnect_Mode_t)(unsigned int BluetoothStackID, SDP_Disconnect_Mode_t SDPDisconnectMode);
#endif

   /* The following function is responsible for Disconnecting a Remote  */
   /* SDP Server Connection that is still open.  This function accepts  */
   /* as input the Bluetooth Stack ID of the Bluetooth Stack that has   */
   /* opened a previous SDP Server Connection and the Bluetooth Address */
   /* of an existing Bluetooth SDP Server for which a connection exists.*/
   /* This function is used when the SDP Disconnect Mode is set to      */
   /* dmManual and has NO effect when the SDP Disconnect Mode is set to */
   /* dmAutomatic.  This function returns zero if successful, or a      */
   /* negative return error code if there was an error.                 */
BTPSAPI_DECLARATION int BTPSAPI SDP_Disconnect_Server(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_SDP_Disconnect_Server_t)(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);
#endif

#endif
