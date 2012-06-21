/******************************************************************************/
/* Copyright 2011 Stonestreet One, LLC                                        */
/* All Rights Reserved.                                                       */
/* Licensed under the Bluetopia(R) Clickwrap License Agreement from TI        */
/*                                                                            */
/*  BTPSKRNL.c - Stellaris(R) Bluetooth Stack OS Kernel Abstraction           */
/*               Implementation for Stonestreet One, LLC Bluetopia(R)         */
/*               Bluetooth Protocol Stack.                                    */
/*                                                                            */
/******************************************************************************/
#include <string.h>

#include "SafeRTOS/SafeRTOS_API.h"  /* SafeRTOS Kernal Prototypes/Constants.  */
#include "SafeRTOS/queue.h"         /* SafeRTOS Queue Prototypes/Constants.   */

#include "BTPSKRNL.h"      /* BTPS Kernel Prototypes/Constants.               */
#include "BTTypes.h"       /* BTPS internal data types.                       */

#include "bt_ucfg.h"       /* BTPS Kernel user configuration constants.       */

#include "utils/ustdlib.h" /* StellarisWare usprintf()/uvnsprintv().          */

   /* The following constant represents the number of bytes that are    */
   /* displayed on an individual line when using the DumpData()         */
   /* function.                                                         */
#define MAXIMUM_BYTES_PER_ROW                           (16)

   /* Defines the maximum number of bytes that will be allocated by the */
   /* kernel abstraction module to support allocations.                 */
   /* * NOTE * This module declares a memory array of this size (in     */
   /*          bytes) that will be used by this module for memory       */
   /*          allocation.                                              */
#ifndef MEMORY_BUFFER_SIZE

   #define MEMORY_BUFFER_SIZE                           (24*1024)

#endif

   /* Defines the maximum number of event bit masks supported           */
   /* simultaneously by this module.                                    */
#define MAXIMUM_EVENT_MASK_COUNT                        (31)

   /* The following MACRO maps Timeouts (specified in Milliseconds) to  */
   /* System Ticks that are required by the Operating System Timeout    */
   /* functions (Waiting only).                                         */
#define MILLISECONDS_TO_TICKS(_x)                       (_x)

   /* The following MACRO is used to align a pointer on the next 8 bytes*/
   /* boundary.                                                         */
#define ALLIGN_8(_x)          (((unsigned long)&(((unsigned char *)(_x))[8])) & ~7)

   /* Denotes the priority of the thread being created using the thread */
   /* create function.                                                  */
#ifndef DEFAULT_THREAD_PRIORITY

   #define DEFAULT_THREAD_PRIORITY                      (3)

#endif

#define BTPS_INVALID_HANDLE_VALUE                       ((unsigned long *)-1)

    /* The following type declaration represents the entire state       */
    /* information for a Mutex type.  This stucture is used with all of */
    /* the Muxtex functions contained in this module.                   */
typedef struct _tagMutexHeader_t
{
   Mutex_t             SemaphoreHandle;
   ThreadHandle_t      Owner;
   int                 Count;
   signed char        *Buffer;
   unsigned long long  AlignmentBuffer[(portQUEUE_OVERHEAD_BYTES+1) >> 2];
} MutexHeader_t;

   /* The following type declaration represents the entire state        */
   /* information for an Event_t.  This structure is used with all of   */
   /* the Event functions contained in this module.                     */
typedef struct _tagEventHeader_t
{
   Event_t             EventHandle;
   signed char        *Buffer;
   unsigned long long  AlignmentBuffer[(portQUEUE_OVERHEAD_BYTES+2) >> 2];
} EventHeader_t;

   /* The following structure is a container structure that exists to   */
   /* map the OS Thread Function to the BTPS Kernel Thread Function (it */
   /* doesn't totally map in a one to one relationship/mapping).        */
typedef struct _tagThreadWrapperInfo_t
{
   ThreadHandle_t Thread;
   Thread_t       ThreadFunction;
   void          *ThreadParameter;
   signed char    ThreadStack[1];
} ThreadWrapperInfo_t;

   /* The following MACRO is a utility MACRO that exists to aid code    */
   /* readability to Determine the size (in Bytes) of an Thread Header  */
   /* structure based upon the number of RAW Bytes that are required by */
   /* for the ThreadStack member.  The first parameter to this MACRO is */
   /* the size (in Bytes) of the Thread Stack that will be included as  */
   /* part of this structure.                                           */
   /* * NOTE * This MACRO adds an additional 8 bytes to the size because*/
   /*          the stack must be alligned on an 8 bytes boundary.  The  */
   /*          maximum amount that the start of the stack can be        */
   /*          adjusted is 8 bytes.                                     */
#define CALCULATE_THREAD_HEADER(_x)    ((sizeof(ThreadWrapperInfo_t)-sizeof(unsigned char))+(_x)+8)

   /* The following type declaration represents the entire state        */
   /* information for a Mailbox.  This structure is used with all of    */
   /* the Mailbox functions contained in this module.                   */
typedef struct _tagMailboxHeader_t
{
   Event_t       Event;
   Mutex_t       Mutex;
   unsigned int  HeadSlot;
   unsigned int  TailSlot;
   unsigned int  OccupiedSlots;
   unsigned int  NumberSlots;
   unsigned int  SlotSize;
   void         *Slots;
} MailboxHeader_t;

   /* The following type definition defines a OSAL Kernel API Memory    */
   /* The following enumerates the states of a heap fragment.           */
typedef enum {fsFree, fsInUse} FragmentState_t;

   /* The following defines a type that is the size in bytes of the     */
   /* desired alignment of each datra fragment.                         */
typedef unsigned int Alignment_t;

   /* The following structure is used to allign data fragments on a     */
   /* specified memory boundary.                                        */
typedef union _tagAlignmentStruct_t
{
   Alignment_t   AlignmentValue;
   unsigned char ByteValue;
} AlignmentStruct_t;

   /* The following defines the structure that describes a memory       */
   /* fragment.                                                         */
typedef struct _tagHeapInfo_t
{
   struct _tagHeapInfo_t *Prev;
   struct _tagHeapInfo_t *Next;
   FragmentState_t        FragmentState;
   unsigned int           Size;
   AlignmentStruct_t      Data;
} HeapInfo_t;

#define HEAP_INFO_DATA_SIZE(_x)              (BTPS_STRUCTURE_OFFSET(HeapInfo_t, Data)+(_x))

   /* The following defines the byte boundary size that has been        */
   /* specified size if the alignment data.                             */
#define ALIGNMENT_SIZE                             sizeof(Alignment_t)

   /* The following defines the size in bytes of a data fragment that is*/
   /* considered a large value.  Allocations that are equal to and      */
   /* larger than this value will be allocated from the end of the      */
   /* buffer.                                                           */
#define LARGE_SIZE                                                1024

   /* The following defines the minimum size of a fragment taht is      */
   /* considered useful.  The value is used when trying to determine if */
   /* a fragment that is larger then the requested size can be broken   */
   /* into 2 framents leaving a fragment that is of the requested size  */
   /* and one that is at least as larger as the MINIMUM_MEMORY_SIZE.    */
#define MINIMUM_MEMORY_SIZE                                         16

   /* The following defines the number of bytes that the debug buffer   */
   /* will contain.                                                     */
#define MAX_DEBUG_MSG_LENGTH                                       256
#define DEBUG_BUFFER_SIZE                  (MAX_DEBUG_MSG_LENGTH << 1)

   /* The following structure is used to manage the debug output buffer.*/
typedef struct _tagDebug_Buffer_t
{
   int InIndex;
   int OutIndex;
   int BufferSize;
   int NumFreeBytes;
   char Buffer[DEBUG_BUFFER_SIZE];
} Debug_Buffer_t;

   /* The following variable is used to guard against multiple threads  */
   /* attempting to use global variables declared in this module.       */
static MutexHeader_t KernelMutexHeader;
static Mutex_t       KernelMutex;

static Mutex_t       IOMutex = NULL;

   /* Declare a buffer that will be used to store the Kernel Mutex      */
   /* Information.  A seperate uffer is needed because this Mutex will  */
   /* be used outside of the module being initialized (and thus, cannot */
   /* use the module's heap).  Note that we declare this as an unsigned */
   /* long buffer so that we can force alignment to be correct.         */
static unsigned long KernelMutexBuffer[((portQUEUE_OVERHEAD_BYTES+1)/sizeof(unsigned long)) + 1];

   /* Declare a buffer to use for the Heap.  Note that we declare this  */
   /* as an unsigned long buffer so that we can force alignment to be   */
   /* correct.                                                          */
static unsigned long MemoryBuffer[MEMORY_BUFFER_SIZE/sizeof(unsigned long) + 1];

static Debug_Buffer_t               DebugBuffer;
static BTPS_MessageOutputCallback_t MessageOutputCallback;

static HeapInfo_t                  *HeapHead;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static unsigned long DebugZoneMask = (unsigned long)DEBUG_ZONES;

   /* Internal Function Prototypes.                                     */
static void HeapInit(void);
static void *_Malloc(unsigned long Size);
static void _MemFree(void *MemoryPtr);
static Byte_t ConsoleWrite(char *Message, int Length);
static void ThreadWrapper(void *UserData);

   /* The following function is used to send a string of characters to  */
   /* the Console or Output device.  The function takes as its first    */
   /* parameter a pointer to a string of characters to be output.  The  */
   /* second parameter indicates the number of bytes in the character   */
   /* string that is to be output.                                      */
static Byte_t ConsoleWrite(char *Message, int Length)
{
   int  cnt;
   char ch;

   /* Check to make sure that the Debug String appears to be semi-valid.*/
   if((Message) && (Length) && (MessageOutputCallback))
   {
      if(IOMutex)
      {
         /* Wait for Access to the buffer.                              */
         BTPS_WaitMutex(IOMutex, BTPS_INFINITE_WAIT);

         /* Check to see if we need to add a <CR><LF> at then end if the   */
         /* string.                                                        */
         /* * NOTE * If the string ends with a \f, then no <CR><LF> will be*/
         /*          added and the \f will be removed.  This is used to    */
         /*          concatinate strings via 2 seperate Writes.            */
         ch = Message[Length-1];
         if(ch == '\f')
            Length--;

         /* Spin until there is enough room in the buffer to hold the      */
         /* message an possible a <CR><LF> sequence.                       */
         while(DebugBuffer.NumFreeBytes < (Length+2))
            BTPS_Delay(1);

         /* Determine the number of bytes that we can sequencially place   */
         /* into the buffer before we wrap.                                */
         cnt = DebugBuffer.BufferSize - DebugBuffer.InIndex;
         if(cnt > Length)
         {
            /* Copy the data to the buffer and update the Index.           */
            BTPS_MemCopy(&(DebugBuffer.Buffer[DebugBuffer.InIndex]), Message, Length);
            DebugBuffer.InIndex += Length;
         }
         else
         {
            /* Copy the data to the buffer handling the buffer wrap and    */
            /* update the Index.                                           */
            BTPS_MemCopy(&(DebugBuffer.Buffer[DebugBuffer.InIndex]), Message, cnt);
            BTPS_MemCopy(DebugBuffer.Buffer, &(Message[cnt]), (Length-cnt));
            DebugBuffer.InIndex = (Length-cnt);
         }

         /* Check to see if we should wrap the buffer.                     */
         if(DebugBuffer.InIndex == DebugBuffer.BufferSize)
            DebugBuffer.InIndex = 0;

         /* If the character is a <FF> the add nothing.                    */
         if(ch != '\f')
         {
            /* Add a <CR> to the string.                                   */
            DebugBuffer.Buffer[DebugBuffer.InIndex++] = '\r';
            if(DebugBuffer.InIndex == DebugBuffer.BufferSize)
               DebugBuffer.InIndex = 0;

            /* Increase the count of the number of bytes in the buffer.    */
            Length++;

            /* If the string did not end in a <CR> add it.                 */
            if(ch != '\n')
            {
               DebugBuffer.Buffer[DebugBuffer.InIndex++] = '\n';
               if(DebugBuffer.InIndex == DebugBuffer.BufferSize)
                  DebugBuffer.InIndex = 0;

               /* Increase the count of the number of bytes in the buffer. */
               Length++;
            }
         }

         taskENTER_CRITICAL();

         /* Update the number of free bytes in the buffer.                 */
         DebugBuffer.NumFreeBytes -= Length;

         taskEXIT_CRITICAL();

         /* Release the Mutex                                           */
         BTPS_ReleaseMutex(IOMutex);
      }

   }

   return(0);
}

   /* The following function is used to initialize the heap structure.  */
   /* The function takes no parameters and returns no status.           */
static void HeapInit(void)
{
   HeapHead                = (HeapInfo_t *)MemoryBuffer;
   HeapHead->FragmentState = fsFree;
   HeapHead->Size          = sizeof(MemoryBuffer)-HEAP_INFO_DATA_SIZE(0);
   HeapHead->Next          = HeapHead;
   HeapHead->Prev          = HeapHead;
}

   /* The following function is used to allocate a fragment of memory   */
   /* from a large buffer.  The function takes as its parameter the size*/
   /* in bytes of the fragment to be allocated.  The function tries to  */
   /* avoid fragmentation by obtaining memory requests larger than      */
   /* LARGE_SIZE from the end of the buffer, while small fragments are  */
   /* taken from the start of the buffer.                               */
static void *_Malloc(unsigned long Size)
{
   void       *ret_val;
   HeapInfo_t *HeapInfoPtr;

   /* Verify that the heap has been initialized.                        */
   if(!HeapHead)
      HeapInit();

   /* Next make sure that the caller is actually requesting memory to be*/
   /* allocated.                                                        */
   if((Size) && (HeapHead))
   {
      /* Initialize the info pointer to the beginning of the list.   */
      HeapInfoPtr = HeapHead;

      /* Make the size a multiple of the Alignment Size.             */
      if(Size % ALIGNMENT_SIZE)
         Size += (ALIGNMENT_SIZE-(Size % ALIGNMENT_SIZE));

      /* Loop until we have looped back to the start of the list.    */
      while(((Size >= LARGE_SIZE) && (HeapInfoPtr->Prev != HeapHead)) || ((Size < LARGE_SIZE) && (HeapInfoPtr->Next != HeapHead)))
      {
         /* Check to see if the current entry is free and is large   */
         /* enough to hold the data being requested.                 */
         if((HeapInfoPtr->FragmentState == fsInUse) || (HeapInfoPtr->Size < Size))
         {
            /* If the requested size is larger then the limit then   */
            /* search backwards for an available buffer, else go     */
            /* forward.  This will hopefully help to reduce          */
            /* fragmentataion problems.                              */
            if(Size >= LARGE_SIZE)
               HeapInfoPtr = HeapInfoPtr->Prev;
            else
               HeapInfoPtr = HeapInfoPtr->Next;
         }
         else
            break;
      }

      /* Check to see if we are pointing to a block that is large    */
      /* enough for the request.                                     */
      if((HeapInfoPtr->FragmentState == fsFree) && (HeapInfoPtr->Size >= Size))
      {
         /* Check to see if we need to slit this into two entries.   */
         /* * NOTE * If there is not enough room to make another     */
         /*          entry then we will not adjust the size of this  */
         /*          entry to match the amount requested.            */
         if(HeapInfoPtr->Size > (Size + HEAP_INFO_DATA_SIZE(MINIMUM_MEMORY_SIZE)))
         {
            /* There will be enough left over to create another      */
            /* entry.  We need to insert a new entry into the list,  */
            /* so create a new entry and link them by the Previous   */
            /* and Next pointers.                                    */
            if(Size >= LARGE_SIZE)
               Size = (HeapInfoPtr->Size - HEAP_INFO_DATA_SIZE(Size));

            ret_val                                = &((unsigned char *)HeapInfoPtr)[HEAP_INFO_DATA_SIZE(Size)];
            ((HeapInfo_t *)ret_val)->Next          = HeapInfoPtr->Next;
            ((HeapInfo_t *)ret_val)->Prev          = HeapInfoPtr;
            ((HeapInfo_t *)ret_val)->FragmentState = (Size < LARGE_SIZE)?fsFree:fsInUse;
            ((HeapInfo_t *)ret_val)->Size          = (HeapInfoPtr->Size - HEAP_INFO_DATA_SIZE(Size));

            (HeapInfoPtr->Next)->Prev  = (HeapInfo_t *)ret_val;
            HeapInfoPtr->Next          = (HeapInfo_t *)ret_val;
            HeapInfoPtr->Size          = Size;
            if(Size < LARGE_SIZE)
               HeapInfoPtr->FragmentState = fsInUse;
            else
            {
               HeapInfoPtr->FragmentState = fsFree;
               HeapInfoPtr                = (HeapInfo_t *)ret_val;
            }
         }
         else
         {
            /* Mark the buffer as In Use.                            */
            HeapInfoPtr->FragmentState = fsInUse;
         }

         /* Get the address of the start of RAM.                     */
         ret_val = (void *)&HeapInfoPtr->Data;
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The following function is used to free memory that was previously */
   /* allocated with _Malloc.  The function takes as its parameter a    */
   /* pointer to the memory that was allocated.  The pointer is used to */
   /* locate the structure of information that describes the allocated  */
   /* fragment.  The function tries to a verify that the structure is a */
   /* valid fragment structure before the memory is freed.  When a      */
   /* fragment is freed, it may be combined with adjacent fragments to  */
   /* produce a larger free fragment.                                   */
static void _MemFree(void *MemoryPtr)
{
   HeapInfo_t *HeapInfoPtr;

   /* Verify that the parameter passed in appears valid.                */
   if((MemoryPtr) && (HeapHead))
   {
      /* Get a pointer to the Heap Info.                                */
      HeapInfoPtr = (HeapInfo_t *)(((unsigned char *)MemoryPtr)-HEAP_INFO_DATA_SIZE(0));
      if(HeapInfoPtr->FragmentState == fsInUse)
      {
         /* Verify that the list links appear correct.                  */
         if(((HeapInfoPtr->Prev)->Next == HeapInfoPtr) && ((HeapInfoPtr->Next)->Prev == HeapInfoPtr))
         {
            /* Check to see if the previous block is free and we can    */
            /* combine the two blocks.                                  */
            if((HeapInfoPtr != HeapHead) && ((HeapInfoPtr->Prev)->FragmentState == fsFree))
            {
               (HeapInfoPtr->Prev)->Next  = HeapInfoPtr->Next;
               (HeapInfoPtr->Prev)->Size += HEAP_INFO_DATA_SIZE(HeapInfoPtr->Size);
               HeapInfoPtr                = HeapInfoPtr->Prev;
               (HeapInfoPtr->Next)->Prev  = HeapInfoPtr;
            }

            /* Check to see if the next block is free and we can combine*/
            /* the two blocks.                                          */
            if((HeapInfoPtr->Next != HeapHead) && ((HeapInfoPtr->Next)->FragmentState == fsFree))
            {
               HeapInfoPtr->Size        += HEAP_INFO_DATA_SIZE((HeapInfoPtr->Next)->Size);
               HeapInfoPtr->Next         = (HeapInfoPtr->Next)->Next;
              (HeapInfoPtr->Next)->Prev  = HeapInfoPtr;
            }

            /* Mark this frament as Free to Use.                        */
            HeapInfoPtr->FragmentState = fsFree;
         }
      }
   }
}

   /* The following function is a function that represents the native   */
   /* thread type for the operating system.  This function does nothing */
   /* more than to call the BTPSKRNL Thread function of the specified   */
   /* format (parameters/calling convention/etc.).  The UserData        */
   /* parameter that is passed to this function is a pointer to a       */
   /* ThreadWrapperInfo_t structure which will contain the actual       */
   /* BTPSKRNL Thread Information.  This function will free this pointer*/
   /* using the BTPS_FreeMemory() function (which means that the Thread */
   /* Information structure needs to be allocated with the              */
   /* BTPS_AllocateMemory() function.                                   */
static void ThreadWrapper(void *UserData)
{
   if(UserData)
   {
      ((*(((ThreadWrapperInfo_t *)UserData)->ThreadFunction))(((ThreadWrapperInfo_t *)UserData)->ThreadParameter));

      /* We are not allowed to return from the thread and we can not    */
      /* release the memory that was allocated for it, so will will go  */
      /* to sleep forever.                                              */
      BTPS_FreeMemory(((ThreadWrapperInfo_t *)UserData));

      xTaskDelete(NULL);
   }
}

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
void BTPSAPI BTPS_Delay(unsigned long MilliSeconds)
{
   /* Simply wrap the OS supplied TaskDelay function.                   */
   if(MilliSeconds == BTPS_INFINITE_WAIT)
   {
      while(TRUE)
         xTaskDelay(MilliSeconds);
   }
   else
      xTaskDelay(MilliSeconds / portTICK_RATE_MS);
}

   /* The following function is responsible for creating an actual      */
   /* Mutex (Binary Semaphore).  The Mutex is unique in that if a       */
   /* Thread already owns the Mutex, and it requests the Mutex again    */
   /* it will be granted the Mutex.  This is in Stark contrast to a     */
   /* Semaphore that will block waiting for the second acquisition of   */
   /* the Sempahore.  This function accepts as input whether or not     */
   /* the Mutex is initially Signalled or not.  If this input parameter */
   /* is TRUE then the caller owns the Mutex and any other threads      */
   /* waiting on the Mutex will block.  This function returns a NON-NULL*/
   /* Mutex Handle if the Mutex was successfully created, or a NULL     */
   /* Mutex Handle if the Mutex was NOT created.  If a Mutex is         */
   /* successfully created, it can only be destroyed by calling the     */
   /* BTPS_CloseMutex() function (and passing the returned Mutex        */
   /* Handle).                                                          */
Mutex_t BTPSAPI BTPS_CreateMutex(Boolean_t CreateOwned)
{
   MutexHeader_t *ret_val;

   /* Before proceeding allocate memory for the mutex header and verify */
   /* that the allocate was successful.                                 */
   if((ret_val = (MutexHeader_t *)BTPS_AllocateMemory(sizeof(MutexHeader_t))) != NULL)
   {
      /* Initialize the memory that was allocated.                      */
      BTPS_MemInitialize(ret_val, 0, sizeof(MutexHeader_t));
      ret_val->Owner = BTPS_INVALID_HANDLE_VALUE;

      /* Make sure that the Buffer is located on an 8 byte boundary.    */
      ret_val->Buffer = (signed char *)ALLIGN_8(ret_val->AlignmentBuffer);
      if(xQueueCreate(ret_val->Buffer, portQUEUE_OVERHEAD_BYTES, 1, 0, &(ret_val->SemaphoreHandle)) == pdPASS)
      {
         /* Check to see if we need to own the Mutex.                   */
         if(!CreateOwned)
         {
            xQueueSend((xQueueHandle)(ret_val->SemaphoreHandle), NULL, (portTickType)0);
         }
         else
         {
            ((MutexHeader_t *)ret_val)->Count = 1;
            ((MutexHeader_t *)ret_val)->Owner = BTPS_CurrentThreadHandle();
         }
      }
      else
      {
         /* An error occurred while attempting to create the os supplied*/
         /* mutex, free the previously allocated resources and return   */
         /* with and error.                                             */
         DBG_MSG(DBG_ZONE_BTPSKRNL,("CreateMutex Error\n"));
         BTPS_FreeMemory(ret_val);
         ret_val = NULL;
      }
   }
   else
      ret_val = NULL;

   return((Mutex_t)ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Mutex to become free.  This function accepts as input   */
   /* the Mutex Handle to wait for, and the Timeout (specified in       */
   /* Milliseconds) to wait for the Mutex to become available.  This    */
   /* function returns TRUE if the Mutex was successfully acquired and  */
   /* FALSE if either there was an error OR the Mutex was not           */
   /* acquired in the specified Timeout.  It should be noted that       */
   /* Mutexes have the special property that if the calling Thread      */
   /* already owns the Mutex and it requests access to the Mutex again  */
   /* (by calling this function and specifying the same Mutex Handle)   */
   /* then it will automatically be granted the Mutex.  Once a Mutex    */
   /* has been granted successfully (this function returns TRUE), then  */
   /* the caller MUST call the BTPS_ReleaseMutex() function.            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
Boolean_t BTPSAPI BTPS_WaitMutex(Mutex_t Mutex, unsigned long Timeout)
{
   long           Result;
   unsigned long  WaitTime;
   Boolean_t      ret_val = FALSE;
   ThreadHandle_t CurrentThread;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(Mutex)
   {
      /* Check to see if we already own this Mutex.                     */
      CurrentThread = BTPS_CurrentThreadHandle();
      if(CurrentThread != (((MutexHeader_t *)Mutex)->Owner))
      {
         /* The parameters appear to be semi-valid.  Next determine the */
         /* amount of time to wait.                                     */
         if(Timeout == BTPS_INFINITE_WAIT)
            WaitTime = portMAX_DELAY;
         else
            WaitTime = MILLISECONDS_TO_TICKS(Timeout);

         do
         {
            /* Simply call the OS supplied wait function.               */
            Result = xQueueReceive((xQueueHandle)(((MutexHeader_t *)Mutex)->SemaphoreHandle), NULL, WaitTime);
            if(Result == pdPASS)
            {
               ((MutexHeader_t *)Mutex)->Count = 1;
               ((MutexHeader_t *)Mutex)->Owner = CurrentThread;
               ret_val                         = TRUE;
            }
         }while(Result == errSCHEDULER_IS_SUSPENDED);

         /* Check to see there was an error.                            */
         if(Result != pdPASS)
            DBG_MSG(DBG_ZONE_BTPSKRNL,("Wait Mutex Error %d %d\n", Result));
      }
      else
      {
         /* Increment the count of the number of times this Mutex has   */
         /* been acquired.                                              */
         ((MutexHeader_t *)Mutex)->Count++;
         ret_val = TRUE;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for releasing a Mutex that  */
   /* was successfully acquired with the BTPS_WaitMutex() function.     */
   /* This function accepts as input the Mutex that is currently        */
   /* owned.                                                            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
void BTPSAPI BTPS_ReleaseMutex(Mutex_t Mutex)
{
   ThreadHandle_t CurrentThread;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if((Mutex) && (((MutexHeader_t *)Mutex)->Count))
   {
      /* Check to see if we already own this Mutex.                     */
      CurrentThread = BTPS_CurrentThreadHandle();
      if(CurrentThread == (((MutexHeader_t *)Mutex)->Owner))
      {
         /* Check to see if we should actually release the Mutex.       */
         if(--(((MutexHeader_t *)Mutex)->Count) == 0)
         {
            /* Flag that no one owns the Mutex *BEFORE* we signal that  */
            /* it is ready for consumption (to avoid a race condition   */
            /* of the Mutex owner being trashed).                       */
            ((MutexHeader_t *)Mutex)->Owner = BTPS_INVALID_HANDLE_VALUE;

            /* The parameters appear to be semi-valid.  Simply attempt  */
            /* to use the os supplied release function.                 */
            if(xQueueSend((xQueueHandle)(((MutexHeader_t *)Mutex)->SemaphoreHandle), NULL, (portTickType)0) != pdPASS)
            {
               DBG_MSG(DBG_ZONE_BTPSKRNL,("Queue Send error\n"));
            }
         }
      }
   }
}

   /* The following function is responsible for destroying a Mutex that */
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateMutex() function.  This function accepts as input the  */
   /* Mutex Handle of the Mutex to destroy.  Once this function is      */
   /* completed the Mutex Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMutex() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseMutex(Mutex_t Mutex)
{
   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(Mutex)
   {
      BTPS_FreeMemory(Mutex);
   }
}

   /* The following function is responsible for creating an actual      */
   /* Event.  The Event is unique in that it only has two states.  These*/
   /* states are Signalled and Non-Signalled.  Functions are provided   */
   /* to allow the setting of the current state and to allow the        */
   /* option of waiting for an Event to become Signalled.  This function*/
   /* accepts as input whether or not the Event is initially Signalled  */
   /* or not.  If this input parameter is TRUE then the state of the    */
   /* Event is Signalled and any BTPS_WaitEvent() function calls will   */
   /* immediately return.  This function returns a NON-NULL Event       */
   /* Handle if the Event was successfully created, or a NULL Event     */
   /* Handle if the Event was NOT created.  If an Event is successfully */
   /* created, it can only be destroyed by calling the BTPS_CloseEvent()*/
   /* function (and passing the returned Event Handle).                 */
Event_t BTPSAPI BTPS_CreateEvent(Boolean_t CreateSignalled)
{
   EventHeader_t *ret_val;
   Byte_t         Msg;
   long           Result;

   /* Before proceeding allocate memory for the mutex header and verify */
   /* that the allocate was successful.                                 */
   if((ret_val = (EventHeader_t *)BTPS_AllocateMemory(sizeof(EventHeader_t))) != NULL)
   {
      /* Initialize the memory that was allocated.                      */
      BTPS_MemInitialize(ret_val, 0, sizeof(EventHeader_t));

      /* Make sure that the Buffer is located on an 8 byte boundary.    */
      ret_val->Buffer = (signed char *)ALLIGN_8(ret_val->AlignmentBuffer);
      Result          = xQueueCreate(ret_val->Buffer, portQUEUE_OVERHEAD_BYTES+1, 1, 1, &(ret_val->EventHandle));
      if(Result)
      {
         /* Check to see if we need to own the Mutex.                   */
         if(CreateSignalled)
            xQueueSend(ret_val->EventHandle, &Msg, 0);
      }
      else
      {
         /* An error occurred while attempting to create the os supplied*/
         /* mutex, free the previously allocated resources and return   */
         /* with and error.                                             */
         DBG_MSG(DBG_ZONE_BTPSKRNL,("CreateEvent Error\n"));
         BTPS_FreeMemory(ret_val);
         ret_val = NULL;
      }
   }
   else
      ret_val = NULL;

   return((Event_t)ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Event to become Signalled.  This function accepts as    */
   /* input the Event Handle to wait for, and the Timeout (specified    */
   /* in Milliseconds) to wait for the Event to become Signalled.  This */
   /* function returns TRUE if the Event was set to the Signalled       */
   /* State (in the Timeout specified) or FALSE if either there was an  */
   /* error OR the Event was not set to the Signalled State in the      */
   /* specified Timeout.  It should be noted that Signalls have a       */
   /* special property in that multiple Threads can be waiting for the  */
   /* Event to become Signalled and ALL calls to BTPS_WaitEvent() will  */
   /* return TRUE whenever the state of the Event becomes Signalled.    */
Boolean_t BTPSAPI BTPS_WaitEvent(Event_t Event, unsigned long Timeout)
{
   long          Result;
   Boolean_t     ret_val = FALSE;
   Byte_t        Msg;
   unsigned long WaitTime;

   /* Verify that the parameter passed in appears valid.                */
   if(Event)
   {
      if(Timeout == BTPS_INFINITE_WAIT)
      {
         WaitTime = portMAX_DELAY;
      }
      else
      {
         WaitTime = MILLISECONDS_TO_TICKS(Timeout);
      }

      do
      {
         /* Get a packet from the queue.                                */
         Result = xQueueReceive(((EventHeader_t *)Event)->EventHandle, &Msg, WaitTime);
      }while(Result == errSCHEDULER_IS_SUSPENDED);

      if((Result != pdPASS) && (Result != errQUEUE_EMPTY))
         DBG_MSG(DBG_ZONE_BTPSKRNL,("Wait Event Error: %d, Timeout %d\n", Result, Timeout));
      else
         xQueueSend(((EventHeader_t *)Event)->EventHandle, &Msg, 0);

      ret_val = (Boolean_t)(Result == pdPASS);
   }
   else
     DBG_MSG(DBG_ZONE_BTPSKRNL,("NULL Event"));

   return(ret_val);
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Non-Signalled State.  Once the Event   */
   /* is in this State, ALL calls to the BTPS_WaitEvent() function will */
   /* block until the State of the Event is set to the Signalled State. */
   /* This function accepts as input the Event Handle of the Event to   */
   /* set to the Non-Signalled State.                                   */
void BTPSAPI BTPS_ResetEvent(Event_t Event)
{
   Byte_t Msg;

   /* Reset the event by obtaining the Mutex.                           */
   if(Event)
   {
      while(xQueueReceive(((EventHeader_t *)Event)->EventHandle, &Msg, 0) == errSCHEDULER_IS_SUSPENDED);
   }
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State.  Once the Event is in */
   /* this State, ALL calls to the BTPS_WaitEvent() function will       */
   /* return.  This function accepts as input the Event Handle of the   */
   /* Event to set to the Signalled State.                              */
void BTPSAPI BTPS_SetEvent(Event_t Event)
{
   Byte_t Msg;

   /* Reset the event by obtaining the Mutex.                           */
   if(Event)
   {
      xQueueSend(((EventHeader_t *)Event)->EventHandle, &Msg, 0);
   }
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State from an Interrupt.     */
   /* Once the Event is in this State, ALL calls to the BTPS_WaitEvent()*/
   /* function will return.  This function accepts as input the Event   */
   /* Handle of the Event to set to the Signalled State.                */
void BTPSAPI BTPS_INT_SetEvent(Event_t Event)
{
   Byte_t Msg;
   long   Result;
   long   ret_val;

   /* Reset the event by obtaining the Mutex.                           */
   if(Event)
   {
      ret_val = xQueueSendFromISR(((EventHeader_t *)Event)->EventHandle, &Msg, &Result);

      if((ret_val != pdPASS) && (ret_val != errQUEUE_FULL))
         taskYIELD_FROM_ISR(Result);
   }
}

   /* The following function is responsible for destroying an Event that*/
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateEvent() function.  This function accepts as input the  */
   /* Event Handle of the Event to destroy.  Once this function is      */
   /* completed the Event Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitEvent() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseEvent(Event_t Event)
{
   if(Event)
   {
      /* Set the event to force any outstanding BTPS_WaitEvent calls to */
      /* return.                                                        */
      BTPS_SetEvent(Event);
   }
}

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize)
{
   void  *ret_val;

   /* Next make sure that the caller is actually requesting memory to be*/
   /* allocated.                                                        */
   if(MemorySize)
   {
      /* The application has not registered a OS specific Malloc        */
      /* function, attempt to acquire the Mutex used to guard access to */
      /* the Memory Allocation Procedure.                               */
      if(BTPS_WaitMutex(KernelMutex, BTPS_INFINITE_WAIT))
      {
         ret_val = _Malloc(MemorySize);

         /* All through, release the previously acquired Mutex.         */
         BTPS_ReleaseMutex(KernelMutex);

         if(!ret_val)
         {
            DBG_MSG(DBG_ZONE_BTPSKRNL, ("Alloc Failed: %d\n", MemorySize));
         }
      }
      else
      {
         DBG_MSG(DBG_ZONE_BTPSKRNL, ("Mutex failed\n"));

         ret_val = NULL;
      }
   }
   else
   {
      DBG_MSG(DBG_ZONE_BTPSKRNL,("Invalid size\n"));
      ret_val = NULL;
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for de-allocating a Block   */
   /* of Memory that was successfully allocated with the                */
   /* BTPS_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the BTPS_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use    */
   /* ANY of the Memory pointed to by the Memory Pointer.               */
void BTPSAPI BTPS_FreeMemory(void *MemoryPointer)
{
   /* First make sure that the memory being returned is semi-valid.     */
   if(MemoryPointer)
   {
      /* Next, acquire the Mutex used to guard access to the Memory     */
      /* Allocation Procedure.                                          */
      if(BTPS_WaitMutex(KernelMutex, BTPS_INFINITE_WAIT))
      {
         _MemFree(MemoryPointer);

         /* All through, release the previously acquired Mutex.         */
         BTPS_ReleaseMutex(KernelMutex);
      }
      else
      {
         DBG_MSG(DBG_ZONE_BTPSKRNL,("Mutex failed\n"));
      }
   }
   else
   {
      DBG_MSG(DBG_ZONE_BTPSKRNL,("Invalid Pointer\n"));
   }
}

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
void BTPSAPI BTPS_MemCopy(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcpy() function.                     */
   memcpy(Destination, Source, Size);
}

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
void BTPSAPI BTPS_MemMove(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memmove() function.                    */
   memmove(Destination, Source, Size);
}

   /* The following function is provided to allow a mechanism to fill   */
   /* a block of memory with the specified value.  This function accepts*/
   /* as input a pointer to the Data Buffer (first parameter) that is   */
   /* to filled with the specified value (second parameter).  The       */
   /* final parameter to this function specifies the number of bytes    */
   /* that are to be filled in the Data Buffer.  The Destination        */
   /* Buffer must point to a Buffer that is AT LEAST the size of        */
   /* the Size parameter.                                               */
void BTPSAPI BTPS_MemInitialize(void *Destination, unsigned char Value, unsigned long Size)
{
   /* Simply wrap the C Run-Time memset() function.                     */
   memset(Destination, Value, Size);
}

   /* The following function is provided to allow a mechanism to        */
   /* Compare two blocks of memory to see if the two memory blocks      */
   /* (each of size Size (in bytes)) are equal (each and every byte up  */
   /* to Size bytes).  This function returns a negative number if       */
   /* Source1 is less than Source2, zero if Source1 equals Source2, and */
   /* a positive value if Source1 is greater than Source2.              */
int BTPSAPI BTPS_MemCompare(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcmp() function.                     */
   return(memcmp(Source1, Source2, Size));
}

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
int BTPSAPI BTPS_MemCompareI(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   int           ret_val = 0;
   unsigned char Byte1;
   unsigned char Byte2;
   unsigned int  Index;

   /* Simply loop through each byte pointed to by each pointer and check*/
   /* to see if they are equal.                                         */
   for (Index=0;((Index<Size) && (!ret_val));Index++)
   {
      /* Note each Byte that we are going to compare.                   */
      Byte1 = ((unsigned char *)Source1)[Index];
      Byte2 = ((unsigned char *)Source2)[Index];

      /* If the Byte in the first array is lower case, go ahead and make*/
      /* it upper case (for comparisons below).                         */
      if ((Byte1 >= 'a') && (Byte1 <= 'z'))
         Byte1 = Byte1 - ('a' - 'A');

      /* If the Byte in the second array is lower case, go ahead and    */
      /* make it upper case (for comparisons below).                    */
      if ((Byte2 >= 'a') && (Byte2 <= 'z'))
         Byte2 = Byte2 - ('a' - 'A');

      /* If the two Bytes are equal then there is nothing to do.        */
      if (Byte1 != Byte2)
      {
         /* Bytes are not equal, so set the return value accordingly.   */
         if (Byte1 < Byte2)
            ret_val = -1;
         else
            ret_val = 1;
      }
   }

   /* Simply return the result of the above comparison(s).              */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* copy a source NULL Terminated ASCII (character) String to the     */
   /* specified Destination String Buffer.  This function accepts as    */
   /* input a pointer to a buffer (Destination) that is to receive the  */
   /* NULL Terminated ASCII String pointed to by the Source parameter.  */
   /* This function copies the string byte by byte from the Source      */
   /* to the Destination (including the NULL terminator).               */
void BTPSAPI BTPS_StringCopy(char *Destination, BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strcpy() function.                     */
   strcpy(Destination, Source);
}

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns     */
   /* the number of characters present in the string (NOT including     */
   /* the terminating NULL character).                                  */
unsigned int BTPSAPI BTPS_StringLength(BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strlen() function.                     */
   return(strlen(Source));
}

   /* The following function is provided to allow a means for the       */
   /* programmer to create a seperate thread of execution.  This        */
   /* function accepts as input the Function that represents the        */
   /* Thread that is to be installed into the system as its first       */
   /* parameter.  The second parameter is the size of the Threads       */
   /* Stack (in bytes) required by the Thread when it is executing.     */
   /* The final parameter to this function represents a parameter that  */
   /* is to be passed to the Thread when it is created.  This function  */
   /* returns a NON-NULL Thread Handle if the Thread was successfully   */
   /* created, or a NULL Thread Handle if the Thread was unable to be   */
   /* created.  Once the thread is created, the only way for the Thread */
   /* to be removed from the system is for the Thread function to run   */
   /* to completion.                                                    */
   /* * NOTE * There does NOT exist a function to Kill a Thread that    */
   /*          is present in the system.  Because of this, other means  */
   /*          needs to be devised in order to signal the Thread that   */
   /*          it is to terminate.                                      */
ThreadHandle_t BTPSAPI BTPS_CreateThread(Thread_t ThreadFunction, unsigned int StackSize, void *ThreadParameter)
{
   long                 Result;
   ThreadHandle_t       ret_val;
   signed char         *ThreadStack;
   ThreadWrapperInfo_t *ThreadWrapperInfo;

   /* Wrap the OS Thread Creation function.                             */
   /* * NOTE * Becuase the OS Thread function and the BTPS Thread       */
   /*          function are not necessarily the same, we will wrap the  */
   /*          BTPS Thread within the real OS thread.                   */
   if(ThreadFunction)
   {
      /* First we need to allocate memory for a ThreadWrapperInfo_t     */
      /* structure to hold the data we are going to pass to the thread. */
      if((ThreadWrapperInfo = (ThreadWrapperInfo_t *)BTPS_AllocateMemory(CALCULATE_THREAD_HEADER(configMINIMAL_STACK_SIZE+StackSize))) != NULL)
      {
         /* The Thread Stack must be aligned on an 8 byte boundary.  We */
         /* align the stack by zeroing the address of the 8th element.  */
         ThreadStack = (signed char *)ALLIGN_8(ThreadWrapperInfo->ThreadStack);

         /* Memory allocated, populate the structure with the correct   */
         /* information.                                                */
         ThreadWrapperInfo->ThreadFunction  = ThreadFunction;
         ThreadWrapperInfo->ThreadParameter = ThreadParameter;

         /* Next attempt to create a thread using the default priority. */
         Result = xTaskCreate(ThreadWrapper, NULL, ThreadStack, StackSize, ThreadWrapperInfo, DEFAULT_THREAD_PRIORITY, &ThreadWrapperInfo->Thread);
         if(Result != pdPASS)
         {
            /* An error occurred while attempting to create the thread. */
            /* Free any previously allocated resources and set the      */
            /* return value to indicate and error has occurred.         */
            DBG_MSG(DBG_ZONE_BTPSKRNL, ("xTaskCreate failed."));
            BTPS_FreeMemory(ThreadWrapperInfo);
            ret_val = NULL;
         }
         else
            ret_val = ThreadWrapperInfo->Thread;
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* retrieve the handle of the thread which is currently executing.   */
   /* This function require no input parameters and will return a valid */
   /* ThreadHandle upon success.                                        */
ThreadHandle_t BTPSAPI BTPS_CurrentThreadHandle(void)
{
   ThreadHandle_t ret_val;

   /* Simply return the Current Thread Handle that is executing.  For   */
   /* the ROM code, this is located at the specified address.           */
   ret_val = (ThreadHandle_t)*(unsigned long *)0x20000010;

  return(ret_val);
}

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
Mailbox_t BTPSAPI BTPS_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize)
{
   Mailbox_t        ret_val;
   MailboxHeader_t *MailboxHeader;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if((NumberSlots) && (SlotSize))
   {
      /* Parameters appear semi-valid, so now let's allocate enough     */
      /* Memory to hold the Mailbox Header AND enough space to hold     */
      /* all requested Mailbox Slots.                                   */
      if((MailboxHeader = (MailboxHeader_t *)BTPS_AllocateMemory(sizeof(MailboxHeader_t)+(NumberSlots*SlotSize))) != NULL)
      {
         /* Memory successfully allocated so now let's create an        */
         /* Event that will be used to signal when there is Data        */
         /* present in the Mailbox.                                     */
         if((MailboxHeader->Event = BTPS_CreateEvent(FALSE)) != NULL)
         {
            /* Event created successfully, now let's create a Mutex     */
            /* that will guard access to the Mailbox Slots.             */
            if((MailboxHeader->Mutex = BTPS_CreateMutex(FALSE)) != NULL)
            {
               /* Everything successfully created, now let's initialize */
               /* the state of the Mailbox such that it contains NO     */
               /* Data.                                                 */
               MailboxHeader->NumberSlots   = NumberSlots;
               MailboxHeader->SlotSize      = SlotSize;
               MailboxHeader->HeadSlot      = 0;
               MailboxHeader->TailSlot      = 0;
               MailboxHeader->OccupiedSlots = 0;
               MailboxHeader->Slots         = ((unsigned char *)MailboxHeader) + sizeof(MailboxHeader_t);

               /* All finished, return success to the caller (the       */
               /* Mailbox Header).                                      */
               ret_val                      = (Mailbox_t)MailboxHeader;
            }
            else
            {
               /* Error creating the Mutex, so let's free the Event     */
               /* we created, Free the Memory for the Mailbox itself    */
               /* and return an error to the caller.                    */
               BTPS_CloseEvent(MailboxHeader->Event);

               BTPS_FreeMemory(MailboxHeader);

               ret_val = NULL;
            }
         }
         else
         {
            /* Error creating the Data Available Event, so let's free   */
            /* the Memory for the Mailbox itself and return an error    */
            /* to the caller.                                           */
            BTPS_FreeMemory(MailboxHeader);

            ret_val = NULL;
         }
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

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
Boolean_t BTPSAPI BTPS_AddMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Check to see if this Mailbox specifies a Windows Message Queue */
      /* or not.                                                        */
      if((((MailboxHeader_t *)Mailbox)->NumberSlots) && (((MailboxHeader_t *)Mailbox)->Event))
      {
         /* Since we are going to change the Mailbox we need to acquire */
         /* the Mutex that is guarding access to it.                    */
         if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
         {
            /* Before adding the data to the Mailbox, make sure that the*/
            /* Mailbox is not already full.                             */
            if(((MailboxHeader_t *)Mailbox)->OccupiedSlots < ((MailboxHeader_t *)Mailbox)->NumberSlots)
            {
               /* Mailbox is NOT full, so add the specified User Data to*/
               /* the next available free Mailbox Slot.                 */
               BTPS_MemCopy(&(((unsigned char *)(((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->HeadSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), MailboxData, ((MailboxHeader_t *)Mailbox)->SlotSize);

               /* Update the Next available Free Mailbox Slot (taking   */
               /* into account wrapping the pointer).                   */
               if(++(((MailboxHeader_t *)Mailbox)->HeadSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
                  ((MailboxHeader_t *)Mailbox)->HeadSlot = 0;

               /* Update the Number of occupied slots to signify that   */
               /* there was additional Mailbox Data added to the        */
               /* Mailbox.                                              */
               ((MailboxHeader_t *)Mailbox)->OccupiedSlots++;

               /* Signal the Event that specifies that there is indeed  */
               /* Data present in the Mailbox.                          */
               BTPS_SetEvent(((MailboxHeader_t *)Mailbox)->Event);

               /* Finally, return success to the caller.                */
               ret_val = TRUE;
            }
            else
               ret_val = FALSE;

            /* All finished with the Mailbox, so release the Mailbox    */
            /* Mutex that we currently hold.                            */
            BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
         }
         else
            ret_val = FALSE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

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
Boolean_t BTPSAPI BTPS_WaitMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Check to see if this Mailbox specifies a Windows Message Queue */
      /* or not.                                                        */
      if((((MailboxHeader_t *)Mailbox)->NumberSlots) && (((MailboxHeader_t *)Mailbox)->Event))
      {
         /* Next, we need to block until there is at least one Mailbox  */
         /* Slot occupied by waiting on the Data Available Event.       */
         if(BTPS_WaitEvent(((MailboxHeader_t *)Mailbox)->Event, BTPS_INFINITE_WAIT))
         {
            /* Since we are going to update the Mailbox, we need to     */
            /* acquire the Mutex that guards access to the Mailox.      */
            if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
            {
               /* Let's double check to make sure that there does indeed*/
               /* exist at least one slot with Mailbox Data present in  */
               /* it.                                                   */
               if(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
               {
                  /* Flag success to the caller.                        */
                  ret_val = TRUE;

                  /* Now copy the Data into the Memory Buffer specified */
                  /* by the caller.                                     */
                  BTPS_MemCopy(MailboxData, &((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), ((MailboxHeader_t *)Mailbox)->SlotSize);

                  /* Now that we've copied the data into the Memory     */
                  /* Buffer specified by the caller we need to mark the */
                  /* Mailbox Slot as free.                              */
                  if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
                     ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

                  ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;

                  /* If there is NO more data available in this Mailbox */
                  /* then we need to flag it as such by Resetting the   */
                  /* Mailbox Data available Event.                      */
                  if(!((MailboxHeader_t *)Mailbox)->OccupiedSlots)
                     BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);
               }
               else
               {
                  /* Internal error, flag that there is NO Data present */
                  /* in the Mailbox (by resetting the Data Available    */
                  /* Event) and return failure to the caller.           */
                  BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);

                  ret_val = FALSE;
               }

               /* All finished with the Mailbox, so release the Mailbox */
               /* Mutex that we currently hold.                         */
               BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
            }
            else
               ret_val = FALSE;
         }
         else
         {
            ret_val = FALSE;
         }
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

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
void BTPSAPI BTPS_DeleteMailbox(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback)
{
   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* that was specified appears semi-valid.                            */
   if(Mailbox)
   {
      /* Check to see if a Mailbox Delete Item Callback was specified.  */
      if(MailboxDeleteCallback)
      {
         /* Now loop though all of the occupied slots and call the      */
         /* callback with the slot data.                                */
         while(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
         {
            __BTPSTRY
            {
               (*MailboxDeleteCallback)(&((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]));
            }
            __BTPSEXCEPT(1)
            {
               /* Do Nothing.                                           */
            }

            /* Now that we've called back with the data, we need to     */
            /* advance to the next slot.                                */
            if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

            /* Flag that there is one less occupied slot.               */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;
         }
      }

      /* Mailbox appears semi-valid, so let's free all Events and       */
      /* Mutexes and finally free the Memory associated with the        */
      /* Mailbox itself.                                                */
      if(((MailboxHeader_t *)Mailbox)->Event)
         BTPS_CloseEvent(((MailboxHeader_t *)Mailbox)->Event);

      if(((MailboxHeader_t *)Mailbox)->Mutex)
         BTPS_CloseMutex(((MailboxHeader_t *)Mailbox)->Mutex);

      /* All finished cleaning up the Mailbox, simply free all          */
      /* memory that was allocated for the Mailbox.                     */
      BTPS_FreeMemory(Mailbox);
   }
}

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
void BTPSAPI BTPS_Init(void *UserParam)
{
   long Result;

   /* Initialize the Debug Output Buffer.                               */
   BTPS_MemInitialize(&DebugBuffer, 0, sizeof(Debug_Buffer_t));

   DebugBuffer.BufferSize   = DEBUG_BUFFER_SIZE;
   DebugBuffer.NumFreeBytes = DEBUG_BUFFER_SIZE;

   /* Verify that the parameter passed in appears valid.                */
   if(UserParam)
      MessageOutputCallback = ((BTPS_Initialization_t *)UserParam)->MessageOutputCallback;
   else
      MessageOutputCallback = NULL;

   HeapInit();

   /* Attempt to create the mutex to be used to serialize access to this*/
   /* modules constructs.                                               */
   BTPS_MemInitialize(&KernelMutexHeader, 0, sizeof(MutexHeader_t));
   KernelMutexHeader.Owner  = BTPS_INVALID_HANDLE_VALUE;
   KernelMutexHeader.Buffer = (signed char *)KernelMutexBuffer;
   KernelMutex              = &KernelMutexHeader;
   Result = xQueueCreate((signed char *)KernelMutexBuffer, portQUEUE_OVERHEAD_BYTES, 1, 0, &((MutexHeader_t *)KernelMutex)->SemaphoreHandle);
   if(Result == pdPASS)
   {
      xQueueSend((xQueueHandle)(((MutexHeader_t *)KernelMutex)->SemaphoreHandle), NULL, (portTickType)0);

      /* Create a Mutex to control access to the Debug Output Buffer.   */
      IOMutex = BTPS_CreateMutex(FALSE);
   }
}

   /* The following function is used to cleanup the Platform module.    */
void BTPSAPI BTPS_DeInit(void)
{
}

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
void BTPS_OutputMessage(BTPSCONST char *DebugString, ...)
{
   va_list args;
   char    DebugMsgBuffer[128];

   /* Write out the Data.                                               */
   va_start(args, DebugString);

   uvsnprintf(DebugMsgBuffer, sizeof(DebugMsgBuffer), DebugString, args);

   ConsoleWrite(DebugMsgBuffer, (int)strlen(DebugMsgBuffer));

   va_end(args);
}

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
void BTPSAPI BTPS_SetDebugMask(unsigned long DebugMask)
{
   DebugZoneMask = DebugMask;
}

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
int BTPSAPI BTPS_TestDebugZone(unsigned long Zone)
{
   return(DebugZoneMask & Zone);
}

   /* The following function is responsible for displaying binary debug */
   /* data.  The first parameter to this function is the length of data */
   /* pointed to by the next parameter.  The final parameter is a       */
   /* pointer to the binary data to be  displayed.                      */
int BTPSAPI BTPS_DumpData(unsigned int DataLength, BTPSCONST unsigned char *DataPtr)
{
   int           ret_val;
   char          Buffer[80];
   char         *BufPtr;
   char         *HexBufPtr;
   Byte_t        DataByte;
   unsigned int  Index;
   static char   HexTable[] = "0123456789ABCDEF\n";
   static char   Header1[]  = "       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ";
   static char   Header2[]  = " -----------------------------------------------------------------------\n";

   /* Before proceeding any further, lets make sure that the parameters */
   /* passed to us appear semi-valid.                                   */
   if((DataLength > 0) && (DataPtr != NULL))
   {
      /* The parameters which were passed in appear to be at least      */
      /* semi-valid, next write the header out to the file.             */
      BTPS_OutputMessage(Header1);
      BTPS_OutputMessage(HexTable);
      BTPS_OutputMessage(Header2);

      /* Limit the number of bytes that will be displayed in the dump.  */
      if(DataLength > MAX_DBG_DUMP_BYTES)
      {
         DataLength = MAX_DBG_DUMP_BYTES;
      }

      /* Now that the Header is written out, let's output the Debug     */
      /* Data.                                                          */
      BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
      BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", 0);
      HexBufPtr = Buffer + sizeof(Header1)-1;
      for(Index=0; Index < DataLength;)
      {
         Index++;
         DataByte     = *DataPtr++;
         *BufPtr++    = HexTable[(Byte_t)(DataByte >> 4)];
         *BufPtr      = HexTable[(Byte_t)(DataByte & 0x0F)];
         BufPtr      += 2;
         /* Check to see if this is a printable character and not a     */
         /* special reserved character.  Replace the non-printable      */
         /* characters with a period.                                   */
         *HexBufPtr++ = (char)(((DataByte >= ' ') && (DataByte <= '~') && (DataByte != '\\') && (DataByte != '%'))?DataByte:'.');
         if(((Index % MAXIMUM_BYTES_PER_ROW) == 0) || (Index == DataLength))
         {
            /* We are at the end of this line, so terminate the data    */
            /* compiled in the buffer and send it to the output device. */
            *HexBufPtr++ = '\n';
            *HexBufPtr   = 0;
            BTPS_OutputMessage(Buffer);
            if(Index != DataLength)
            {
               /* We have more to process, so prepare for the next line.*/
               BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
               BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", Index);
               HexBufPtr = Buffer + sizeof(Header1)-1;
            }
            else
            {
               /* Flag that there is no more data.                      */
               HexBufPtr = NULL;
            }
         }
      }
      /* Check to see if there is partial data in the buffer.           */
      if(HexBufPtr > 0)
      {
         /* Terminate the buffer and output the line.                   */
         *HexBufPtr++ = '\n';
         *HexBufPtr   = 0;
         BTPS_OutputMessage(Buffer);
      }
      BTPS_OutputMessage("\n");

      /* Finally, set the return value to indicate success.             */
      ret_val = 0;
   }
   else
   {
      /* An invalid parameter was enterer.                              */
      ret_val = -1;
   }
   return(ret_val);
}

   /* The following function is the Application Idle Hook that should be*/
   /* called during the Idle Task.  This function is responsible for    */
   /* processing any currently queued output information (anything that */
   /* has been queued for writing via the BTPS_OutputMessage() or the   */
   /* BTPS_DumpData() functions.                                        */
void BTPSAPI BTPS_ApplicationIdleHook(void)
{
   /* Verify that a Callback function has been registered.              */
   if(MessageOutputCallback)
   {
      /* Check to see if there are any characters in the Debug Buffer.  */
      if(DebugBuffer.NumFreeBytes != DebugBuffer.BufferSize)
      {
         /* Dispatch the next character to the output function.         */
         MessageOutputCallback(DebugBuffer.Buffer[DebugBuffer.OutIndex++]);
         if(DebugBuffer.OutIndex == DebugBuffer.BufferSize)
            DebugBuffer.OutIndex = 0;

         taskENTER_CRITICAL();
         DebugBuffer.NumFreeBytes++;
         taskEXIT_CRITICAL();
      }
   }
}
