/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  BKRNLAPI.h - Stellaris(R) Bluetooth Stack OS Kernel Abstraction API Type  */
/*               Definitions, Constants and Prototypes for Stonestreet One,   */
/*               LLC Bluetopia(R) Bluetooth Protocol Stack.                   */
/*                                                                            */
/******************************************************************************/
#ifndef __BKRNLAPIH__
#define __BKRNLAPIH__

#include <stdio.h>              /* sprintf() prototype.                       */

#include "BTAPITyp.h"           /* Bluetooth API Type Definitions.            */
#include "BTTypes.h"            /* Bluetooth basic type definitions           */

   /* If a sprintf() replacement has not been specified, go ahead and   */
   /* include the prototype for the one that will be used by default.   */
#ifndef BTPS_SprintF

   #include "utils/ustdlib.h"   /* StellarisWare usprintf() prototype.        */

#endif

   /* Miscellaneous Type definitions that should already be defined,    */
   /* but are necessary.                                                */
#ifndef NULL
   #define NULL ((void *)0)
#endif

#ifndef TRUE
   #define TRUE (1 == 1)
#endif

#ifndef FALSE
   #define FALSE (0 == 1)
#endif

   /* The following preprocessor definitions control the inclusion of   */
   /* debugging output.                                                 */
   /*                                                                   */
   /*    - DEBUG_ENABLED                                                */
   /*         - When defined enables debugging, if no other debugging   */
   /*           preprocessor definitions are defined then the debugging */
   /*           output is logged to a file (and included in the         */
   /*           driver).                                                */
   /*                                                                   */
   /*          - DEBUG_ZONES                                            */
   /*              - When defined (only when DEBUG_ENABLED is defined)  */
   /*                forces the value of this definition (unsigned long)*/
   /*                to be the Debug Zones that are enabled.            */
#define DBG_ZONE_CRITICAL_ERROR           (1 << 0)
#define DBG_ZONE_ENTER_EXIT               (1 << 1)
#define DBG_ZONE_BTPSKRNL                 (1 << 2)
#define DBG_ZONE_GENERAL                  (1 << 3)
#define DBG_ZONE_DEVELOPMENT              (1 << 4)
#define DBG_ZONE_VENDOR                   (1 << 7)

#define DBG_ZONE_ANY                      ((unsigned long)-1)

#ifndef DEBUG_ZONES
   #define DEBUG_ZONES                    DBG_ZONE_CRITICAL_ERROR
#endif

#ifndef MAX_DBG_DUMP_BYTES
   #define MAX_DBG_DUMP_BYTES             (((unsigned int)-1) - 1)
#endif

#ifdef DEBUG_ENABLED

   #define DBG_MSG(_zone_, _x_)           do { if(BTPS_TestDebugZone(_zone_)) BTPS_OutputMessage _x_; } while(0)
   #define DBG_DUMP(_zone_, _x_)          do { if(BTPS_TestDebugZone(_zone_)) BTPS_DumpData _x_; } while(0)

#else

   #define DBG_MSG(_zone_, _x_)
   #define DBG_DUMP(_zone_, _x_)

#endif

   /* The following type definition defines a BTPS Kernel API Mailbox   */
   /* Handle.                                                           */
typedef void *Mailbox_t;

   /* The following MACRO is a utility MACRO that exists to calculate   */
   /* the offset position of a particular structure member from the     */
   /* start of the structure.  This MACRO accepts as the first          */
   /* parameter, the physical name of the structure (the type name, NOT */
   /* the variable name).  The second parameter to this MACRO represents*/
   /* the actual structure member that the offset is to be determined.  */
   /* This MACRO returns an unsigned integer that represents the offset */
   /* (in bytes) of the structure member.                               */
#define BTPS_STRUCTURE_OFFSET(_x, _y)              ((unsigned int )&(((_x *)0)->_y))

   /* The following type declaration represents the Prototype for the   */
   /* function that is passed to the BTPS_DeleteMailbox() function to   */
   /* process all remaining Queued Mailbox Messages.  This allows a     */
   /* mechanism to free any resources that are attached with each       */
   /* queued Mailbox message.                                           */
typedef void (BTPSAPI *BTPS_MailboxDeleteCallback_t)(void *MailboxData);

   /* The following type declaration represents the Prototype for a     */
   /* Scheduler Function.  This function represents the Function that   */
   /* will be executed periodically when passed to the                  */
   /* BTPS_AddFunctionToScheduler() function.                           */
   /* * NOTE * The ScheduleParameter is the same parameter value that   */
   /*          was passed to the BTPS_AddFunctionToScheduler() when     */
   /*          the function was added to the scheduler.                 */
   /* * NOTE * Once a Function is added to the Scheduler there is NO    */
   /*          way to remove it.                                        */
typedef void (BTPSAPI *BTPS_SchedulerFunction_t)(void *ScheduleParameter);

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
BTPSAPI_DECLARATION void BTPSAPI BTPS_Delay(unsigned long MilliSeconds);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_Delay_t)(unsigned long MilliSeconds);
#endif

   /* The following function is responsible for retrieving the current  */
   /* Tick Count of system.  This function returns the System Tick      */
   /* Count in Milliseconds resolution.                                 */
BTPSAPI_DECLARATION unsigned long BTPSAPI BTPS_GetTickCount(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef unsigned long (BTPSAPI *PFN_BTPS_GetTickCount_t)(void);
#endif

   /* The following function is provided to allow a mechanism for       */
   /* adding Scheduler Functions to the Scheduler.  These functions are */
   /* called periodically by the Scheduler (based upon the requested    */
   /* Schedule Period).  This function accepts as input the Scheduler   */
   /* Function to add to the Scheduler, the Scheduler parameter that is */
   /* passed to the Scheduled function, and the Scheduler Period.  The  */
   /* Scheduler Period is specified in Milliseconds.  This function     */
   /* returns TRUE if the function was added successfully or FALSE if   */
   /* there was an error.                                               */
   /* * NOTE * Once a function is added to the Scheduler, it can only   */
   /*          be removed by calling the                                */
   /*          BTPS_DeleteFunctionFromScheduler() function.             */
   /* * NOTE * The BTPS_ExecuteScheduler() function *MUST* be called    */
   /*          ONCE (AND ONLY ONCE) to begin the Scheduler Executing    */
   /*          periodic Scheduled functions (or calling the             */
   /*          BTPS_ProcessScheduler() function repeatedly.             */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BTPS_AddFunctionToScheduler(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter, unsigned int Period);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BTPS_AddFunctionToScheduler_t)(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter, unsigned int Period);
#endif

   /* The following function is provided to allow a mechanism for       */
   /* deleting a Function that has previously been registered with the  */
   /* Scheduler via a successful call to the                            */
   /* BTPS_AddFunctionToScheduler() function.  This function accepts as */
   /* input the Scheduler Function to that was added to the Scheduler,  */
   /* as well as the Scheduler Parameter that was registered.  Both of  */
   /* these values *must* match to remove a specific Scheduler Entry.   */
BTPSAPI_DECLARATION void BTPSAPI BTPS_DeleteFunctionFromScheduler(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_DeleteFunctionFromScheduler_t)(BTPS_SchedulerFunction_t SchedulerFunction, void *SchedulerParameter);
#endif

   /* The following function begins execution of the actual Scheduler.  */
   /* Once this function is called, it NEVER returns.  This function is */
   /* responsible for executing all functions that have been added to   */
   /* the Scheduler with the BTPS_AddFunctionToScheduler() function.    */
BTPSAPI_DECLARATION void BTPSAPI BTPS_ExecuteScheduler(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_ExecuteScheduler_t)(void);
#endif

   /* The following function is provided to allow a mechanism to process*/
   /* the scheduled functions in the scheduler.  This function performs */
   /* the same functionality as the BTPS_ExecuteScheduler() function    */
   /* except that it returns as soon as it has made an iteration through*/
   /* the scheduled functions.  This function is provided for platforms */
   /* that would like to implement their own processing loop and/or     */
   /* scheduler and not rely on the Bluetopia implementation.           */
   /* * NOTE * This function should NEVER be called if the              */
   /*          BTPS_ExecuteScheduler() schema is used.                  */
BTPSAPI_DECLARATION void BTPSAPI BTPS_ProcessScheduler(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_ProcessScheduler_t)(void);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
BTPSAPI_DECLARATION void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void *(BTPSAPI *PFN_BTPS_AllocateMemory_t)(unsigned long MemorySize);
#endif

   /* The following function is responsible for de-allocating a Block   */
   /* of Memory that was successfully allocated with the                */
   /* BTPS_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the BTPS_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use    */
   /* ANY of the Memory pointed to by the Memory Pointer.               */
BTPSAPI_DECLARATION void BTPSAPI BTPS_FreeMemory(void *MemoryPointer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_FreeMemory_t)(void *MemoryPointer);
#endif

   /* The following function is responsible for copying a block of      */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function does not allow the overlapping of the      */
   /*          Source and Destination Buffers !!!!                      */
BTPSAPI_DECLARATION void BTPSAPI BTPS_MemCopy(void *Destination, BTPSCONST void *Source, unsigned long Size);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_MemCopy_t)(void *Destination, BTPSCONST void *Source, unsigned long Size);
#endif

   /* The following function is responsible for moving a block of       */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function DOES allow the overlapping of the          */
   /*          Source and Destination Buffers.                          */
BTPSAPI_DECLARATION void BTPSAPI BTPS_MemMove(void *Destination, BTPSCONST void *Source, unsigned long Size);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_MemMove_t)(void *Destination, BTPSCONST void *Source, unsigned long Size);
#endif

   /* The following function is provided to allow a mechanism to fill   */
   /* a block of memory with the specified value.  This function accepts*/
   /* as input a pointer to the Data Buffer (first parameter) that is   */
   /* to filled with the specified value (second parameter).  The       */
   /* final parameter to this function specifies the number of bytes    */
   /* that are to be filled in the Data Buffer.  The Destination        */
   /* Buffer must point to a Buffer that is AT LEAST the size of        */
   /* the Size parameter.                                               */
BTPSAPI_DECLARATION void BTPSAPI BTPS_MemInitialize(void *Destination, unsigned char Value, unsigned long Size);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_MemInitialize_t)(void *Destination, unsigned char Value, unsigned long Size);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* Compare two blocks of memory to see if the two memory blocks      */
   /* (each of size Size (in bytes)) are equal (each and every byte up  */
   /* to Size bytes).  This function returns a negative number if       */
   /* Source1 is less than Source2, zero if Source1 equals Source2, and */
   /* a positive value if Source1 is greater than Source2.              */
BTPSAPI_DECLARATION int BTPSAPI BTPS_MemCompare(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BTPS_MemCompare_t)(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size);
#endif

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
BTPSAPI_DECLARATION int BTPSAPI BTPS_MemCompareI(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BTPS_MemCompareI_t)(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned int Size);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* copy a source NULL Terminated ASCII (character) String to the     */
   /* specified Destination String Buffer.  This function accepts as    */
   /* input a pointer to a buffer (Destination) that is to receive the  */
   /* NULL Terminated ASCII String pointed to by the Source parameter.  */
   /* This function copies the string byte by byte from the Source      */
   /* to the Destination (including the NULL terminator).               */
BTPSAPI_DECLARATION void BTPSAPI BTPS_StringCopy(char *Destination, BTPSCONST char *Source);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_StringCopy_t)(char *Destination, BTPSCONST char *Source);
#endif

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns     */
   /* the number of characters present in the string (NOT including     */
   /* the terminating NULL character).                                  */
BTPSAPI_DECLARATION unsigned int BTPSAPI BTPS_StringLength(BTPSCONST char *Source);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef unsigned int (BTPSAPI *PFN_BTPS_StringLength_t)(BTPSCONST char *Source);
#endif

   /* The following MACRO definition is provided to allow a mechanism   */
   /* for a C Run-Time Library sprintf() function implementation.  This */
   /* MACRO could be redefined as a function (like the rest of the      */
   /* functions in this file), however more code would be required to   */
   /* implement the variable number of arguments and formatting code    */
   /* then it would be to simply call the C Run-Time Library sprintf()  */
   /* function.  It is simply provided here as a MACRO mapping to allow */
   /* an easy means for a starting place to port this file to other     */
   /* operating systems/platforms.                                      */
#ifndef BTPS_SprintF

   #define BTPS_SprintF usprintf

#endif

   /* The following function is provided to allow a mechanism to create */
   /* a Mailbox.  A Mailbox is a Data Store that contains slots (all    */
   /* of the same size) that can have data placed into (and retrieved   */
   /* from).  Once Data is placed into a Mailbox (via the               */
   /* BTPS_AddMailbox() function, it can be retreived by using the      */
   /* BTPS_WaitMailbox() function.  Data placed into the Mailbox is     */
   /* retrieved in a FIFO method.  This function accepts as input the   */
   /* Maximum Number of Slots that will be present in the Mailbox and   */
   /* the Size of each of the Slots.  This function returns a NON-NULL  */
   /* Mailbox Handle if the Mailbox is successfully created, or a       */
   /* NULL Mailbox Handle if the Mailbox was unable to be created.      */
BTPSAPI_DECLARATION Mailbox_t BTPSAPI BTPS_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Mailbox_t (BTPSAPI *PFN_BTPS_CreateMailbox_t)(unsigned int NumberSlots, unsigned int SlotSize);
#endif

   /* The following function is provided to allow a means to Add data   */
   /* to the Mailbox (where it can be retrieved via the                 */
   /* BTPS_WaitMailbox() function.  This function accepts as input the  */
   /* Mailbox Handle of the Mailbox to place the data into and a        */
   /* pointer to a buffer that contains the data to be added.  This     */
   /* pointer *MUST* point to a data buffer that is AT LEAST the Size   */
   /* of the Slots in the Mailbox (specified when the Mailbox was       */
   /* created) and this pointer CANNOT be NULL.  The data that the      */
   /* MailboxData pointer points to is placed into the Mailbox where it */
   /* can be retrieved via the BTPS_WaitMailbox() function.             */
   /* * NOTE * This function copies from the MailboxData Pointer the    */
   /*          first SlotSize Bytes.  The SlotSize was specified when   */
   /*          the Mailbox was created via a successful call to the     */
   /*          BTPS_CreateMailbox() function.                           */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BTPS_AddMailbox(Mailbox_t Mailbox, void *MailboxData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BTPS_AddMailbox_t)(Mailbox_t Mailbox, void *MailboxData);
#endif

   /* The following function is provided to allow a means to retrieve   */
   /* data from the specified Mailbox.  This function will block until  */
   /* either Data is placed in the Mailbox or an error with the Mailbox */
   /* was detected.  This function accepts as its first parameter a     */
   /* Mailbox Handle that represents the Mailbox to wait for the data   */
   /* with.  This function accepts as its second parameter, a pointer   */
   /* to a data buffer that is AT LEAST the size of a single Slot of    */
   /* the Mailbox (specified when the BTPS_CreateMailbox() function     */
   /* was called).  The MailboxData parameter CANNOT be NULL.  This     */
   /* function will return TRUE if data was successfully retrieved      */
   /* from the Mailbox or FALSE if there was an error retrieving data   */
   /* from the Mailbox.  If this function returns TRUE then the first   */
   /* SlotSize bytes of the MailboxData pointer will contain the data   */
   /* that was retrieved from the Mailbox.                              */
   /* * NOTE * This function copies to the MailboxData Pointer the      */
   /*          data that is present in the Mailbox Slot (of size        */
   /*          SlotSize).  The SlotSize was specified when the Mailbox  */
   /*          was created via a successful call to the                 */
   /*          BTPS_CreateMailbox() function.                           */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BTPS_WaitMailbox(Mailbox_t Mailbox, void *MailboxData);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BTPS_WaitMailbox_t)(Mailbox_t Mailbox, void *MailboxData);
#endif

   /* The following function is a utility function that exists to       */
   /* determine if there is anything queued in the specified Mailbox.   */
   /* This function returns TRUE if there is something queued in the    */
   /* Mailbox, or FALSE if there is nothing queued in the specified     */
   /* Mailbox.                                                          */
BTPSAPI_DECLARATION Boolean_t BTPSAPI BTPS_QueryMailbox(Mailbox_t Mailbox);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_BTPS_QueryMailbox_t)(Mailbox_t Mailbox);
#endif

   /* The following function is responsible for destroying a Mailbox    */
   /* that was created successfully via a successful call to the        */
   /* BTPS_CreateMailbox() function.  This function accepts as input    */
   /* the Mailbox Handle of the Mailbox to destroy.  Once this function */
   /* is completed the Mailbox Handle is NO longer valid and CANNOT be  */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMailbox() functions to fail with an error.  The final    */
   /* parameter specifies an (optional) callback function that is called*/
   /* for each queued Mailbox entry.  This allows a mechanism to free   */
   /* any resources that might be associated with each individual       */
   /* Mailbox item.                                                     */
BTPSAPI_DECLARATION void BTPSAPI BTPS_DeleteMailbox(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_DeleteMailbox_t)(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback);
#endif

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
BTPSAPI_DECLARATION void BTPSAPI BTPS_Init(void *UserParam);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_Init_t)(void *UserParam);
#endif

   /* The following function is used to cleanup the Platform module.    */
BTPSAPI_DECLARATION void BTPSAPI BTPS_DeInit(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_DeInit_t)(void);
#endif

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
BTPSAPI_DECLARATION void BTPSAPI BTPS_OutputMessage(BTPSCONST char *DebugString, ...);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_OutputMessage_t)(BTPSCONST char *DebugString, ...);
#endif

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
BTPSAPI_DECLARATION void BTPSAPI BTPS_SetDebugMask(unsigned long DebugMask);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_BTPS_SetDebugMask_t)(unsigned long DebugMask);
#endif

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
BTPSAPI_DECLARATION int BTPSAPI BTPS_TestDebugZone(unsigned long Zone);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BTPS_TestDebugZone_t)(unsigned long Zone);
#endif

   /* The following function is responsible for writing binary debug    */
   /* data to the specified debug file handle.  The first parameter to  */
   /* this function is the handle of the open debug file to write the   */
   /* debug data to.  The second parameter to this function is the      */
   /* length of binary data pointed to by the next parameter.  The final*/
   /* parameter to this function is a pointer to the binary data to be  */
   /* written to the debug file.                                        */
BTPSAPI_DECLARATION int BTPSAPI BTPS_DumpData(unsigned int DataLength, BTPSCONST unsigned char *DataPtr);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_BTPS_DumpData_t)(unsigned int DataLength, BTPSCONST unsigned char *DataPtr);
#endif

#endif
