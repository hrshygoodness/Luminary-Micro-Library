/*****< hcicommt.h >***********************************************************/
/*      Copyright 2000 - 2011 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HCICOMMT - Serial HCI Driver Layer Types.                                 */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/25/00  T. Thomas      Initial creation.                               */
/******************************************************************************/
#ifndef __HCICOMMTH__
#define __HCICOMMTH__

   /* The following constants represent the Minimum, Maximum, and       */
   /* Values that are used with the Initialization Delay member of the  */
   /* HCI_COMMDriverInformation_t structure.  These Delays are specified*/
   /* in Milliseconds and represent the delay that is to be added       */
   /* between Port Initialization (Open) and the writing of any data to */
   /* the Port.  This functionality was added because it was found that */
   /* some PCMCIA/Compact Flash Cards required a delay between the      */
   /* time the Port was opened and the time when the Card was ready to  */
   /* accept data.  The default is NO Delay (0 Milliseconds).           */
#define HCI_COMM_INFORMATION_INITIALIZATION_DELAY_MINIMUM     0
#define HCI_COMM_INFORMATION_INITIALIZATION_DELAY_MAXIMUM  5000
#define HCI_COMM_INFORMATION_INITIALIZATION_DELAY_DEFAULT     0

   /* The following type declaration defines the HCI Serial Protocol    */
   /* that will be used as the physical HCI Transport Protocol on the   */
   /* actual COM Port that is opened.  This type declaration is used in */
   /* the HCI_COMMDriverInformation_t structure that is required when   */
   /* an HCI COMM Port is opened.                                       */
typedef enum
{
   cpUART,
   cpUART_RTS_CTS,
   cpBCSP,
   cpBCSP_Muzzled
} HCI_COMM_Protocol_t;

   /* The following type declaration represents the structure of all    */
   /* Data that is needed to open an HCI COMM Port.                     */
typedef struct _tagHCI_COMMDriverInformation_t
{
   unsigned int           DriverInformationSize;/* Physical Size of this      */
                                                /* structure.                 */
   unsigned int           COMPortNumber;        /* Physical COM Port Number   */
                                                /* of the Physical COM Port to*/
                                                /* Open.                      */
   unsigned long          BaudRate;             /* Baud Rate to Open COM Port.*/
   HCI_COMM_Protocol_t    Protocol;             /* HCI Protocol that will be  */
                                                /* used for communication over*/
                                                /* Opened COM Port.           */
   unsigned int           InitializationDelay;  /* Time (In Milliseconds) to  */
                                                /* Delay after the Port is    */
                                                /* opened before any data is  */
                                                /* sent over the Port.  This  */
                                                /* member is present because  */
                                                /* some PCMCIA/Compact Flash  */
                                                /* Cards have been seen that  */
                                                /* require a delay because the*/
                                                /* card does not function for */
                                                /* some specified period of   */
                                                /* time.                      */
  char                   *COMDeviceName;        /* Physical Device Name to use*/
                                                /* to override the device to  */
                                                /* open.  If COMPortNumber is */
                                                /* specified to be the        */
                                                /* equivalent of negative 1   */
                                                /* (-1), then this value is   */
                                                /* taken as an absolute name  */
                                                /* and the COM Port Number is */
                                                /* NOT appended to this value.*/
                                                /* If this value is NULL then */
                                                /* the default (compiled) COM */
                                                /* Device Name is used (and   */
                                                /* the COM Port Number is     */
                                                /* appended to the default).  */
} HCI_COMMDriverInformation_t;

   /* The following constant is used with the                           */
   /* HCI_COMM_Driver_Reconfigure_Data_t structure (ReconfigureCommand  */
   /* member) to specify that the Communication parameters are required */
   /* to change.  When specified, the ReconfigureData member will point */
   /* to a valid HCI_COMMDriverInformation_t structure which holds the  */
   /* new parameters.                                                   */
   /* * NOTE * The underlying driver may not support changing all of    */
   /*          specified parameters.  For example, a BCSP enabled port  */
   /*          may not accept being changed to a UART port.             */
#define HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_PARAMETERS   (HCI_DRIVER_RECONFIGURE_DATA_RECONFIGURE_COMMAND_TRANSPORT_START)

#endif
