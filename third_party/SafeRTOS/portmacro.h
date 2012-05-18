/*
	SafeRTOS Copyright (C) Wittenstein High Integrity Systems.

	See projdefs.h for version number information.

	SafeRTOS is distributed exclusively by Wittenstein High Integrity Systems,
	and is subject to the terms of the License granted to your organization,
	including its warranties and limitations on distribution.  It cannot be
	copied or reproduced in any way except as permitted by the License.

	Licenses are issued for each concurrent user working on a specified product
	line.

	WITTENSTEIN high integrity systems is a trading name of WITTENSTEIN
	aerospace & simulation ltd, Registered Office: Brown's Court,, Long Ashton
	Business Park, Yanley Lane, Long Ashton, Bristol, BS41 9LB, UK.
	Tel: +44 (0) 1275 395 600, fax: +44 (0) 1275 393 630.
	E-mail: info@HighIntegritySystems.com
	Registered in England No. 3711047; VAT No. GB 729 1583 15

	http://www.SafeRTOS.com
*/

#ifndef PORTMACRO_H
#define PORTMACRO_H

/* Type definitions. */
#define portCHAR		char
#define portLONG		long
#define portSTACK_TYPE	unsigned portLONG
#define portBASE_TYPE	long

typedef unsigned portLONG portTickType;
#define portMAX_DELAY ( 0xffffffffUL )

/*-----------------------------------------------------------*/

/* Compiler specifics. */
#ifdef __GNUC__
	#define NAKED							__attribute__ ((naked))
#else
	#define NAKED
#endif /* __GNUC__ */

/*-----------------------------------------------------------*/	

/* Architecture specifics. */
#define portSTACK_GROWTH				( -1 )
#define portBYTE_ALIGNMENT				( 4 )
#define portQUEUE_OVERHEAD_BYTES		( 96UL )
#define portCONTEXT_SIZE_BYTES			( ( unsigned portLONG ) 17 * sizeof( portSTACK_TYPE ) )
#define portBYTE_ALIGNMENT_MASK			( 0x0003UL )
#define portIMPLEMENTED_BASEPRI_BITS	( 0xE0 ) /* 8 priorities in the top 3 bits of basepri. */

/*-----------------------------------------------------------*/	

/* Scheduler utilities. */
#define portNVIC_INT_CTRL				( ( volatile unsigned portLONG *) 0xe000ed04UL )
#define portNVIC_PENDSVSET				( 0x10000000UL )

extern void vPortYield( void );
#define portYIELD()					vPortYield()

/* Yielding from an ISR should set the pend bit and nothing else.  The yield
will occur when basepri returns back to 0. */
#define portYIELD_FROM_ISR( xSwitchRequired )			\
{														\
	if( xSwitchRequired )								\
    {													\
		*( portNVIC_INT_CTRL ) = portNVIC_PENDSVSET; 	\
	}													\
}
#define portKERNEL_INTERRUPT_PRIORITY 	( ( unsigned portLONG ) 255 )
#define portSYSCALL_INTERRUPT_PRIORITY  ( ( unsigned portLONG ) 191 ) /* equivalent to 0xa0, or priority 5. */
/*-----------------------------------------------------------*/


/* Critical section management. */


extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
extern unsigned portLONG ulPortSetInterruptMaskFromISR( void ) NAKED;
extern void vPortClearInterruptMaskFromISR( unsigned portLONG ulOriginalMask ) NAKED;


#define portENTER_CRITICAL()		vPortEnterCritical()
#define portEXIT_CRITICAL()			vPortExitCritical()

/* 
 * Set basepri to portSYSCALL_INTERRUPT_PRIORITY without effecting other
 * registers.  r0 is clobbered.
 */
#ifdef __GNUC__

	#define portSET_INTERRUPT_MASK()						\
		__asm volatile										\
		(													\
			"	mov r0, %0								\n"	\
			"	msr basepri, r0							\n" \
			::"i"(portSYSCALL_INTERRUPT_PRIORITY):"r0"		\
		)

#elif defined(__ARMCC_VERSION)

	extern void portSET_INTERRUPT_MASK(void);
	
#elif defined(__IAR_SYSTEMS_ICC__)

	#define portSET_INTERRUPT_MASK()						\
		__asm												\
		(													\
			"	mov r0, #191							\n"	\
			"	msr basepri, r0							\n"	\
		)

#else

	#error Unknown compiler!
	
#endif
	
/*
 * Set basepri back to 0 without effective other registers.
 * r0 is clobbered.
 */
#ifdef __GNUC__

	#define portCLEAR_INTERRUPT_MASK()			\
		__asm volatile							\
		(										\
			"	mov r0, #0					\n"	\
			"	msr basepri, r0				\n"	\
			:::"r0"								\
		)

#elif defined(__ARMCC_VERSION)

	extern void portCLEAR_INTERRUPT_MASK(void);

#elif defined(__IAR_SYSTEMS_ICC__)

	#define portCLEAR_INTERRUPT_MASK()			\
		__asm									\
		(										\
			"	mov r0, #0					\n"	\
			"	msr basepri, r0				\n"	\
		)

#else

	#error Unknown compiler!
	
#endif

/* These two macros are defined separately as they appear in the core code and
are not (yet) implemented for all ports. */
#define portSET_INTERRUPT_MASK_FROM_ISR()	ulPortSetInterruptMaskFromISR()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR( ulOriginalMask )	vPortClearInterruptMaskFromISR( ulOriginalMask )

/*-----------------------------------------------------------*/

/* Now replacements for memory copy and memory zeroing - these are 
   memcpy() and memset for other ports. */

/*
 * vPortZeroWordAlignedBuffer is used for zeroing out memory blocks. We know the last
 * parameter is a constant (sizeof) and the middle parameter is 0. 
 *
 * Ports not defining this function can use:
 * #define vPortZeroWordAlignedBuffer( pvDestination, xValue, uxLength ) (void) memset( pvDestination, xValue, uxLength )
 */
void vPortZeroWordAlignedBuffer( void *pvDestination, portBASE_TYPE xValue, unsigned portBASE_TYPE uxLength ) NAKED;

/* 
 * vPortCopyTaskName is used to copy a string into the pcTaskName field of a TCB.
 *
 * Ports not defining this function can use:
 * #define vPortCopyTaskName( pvDestination, pvSource, uxLength ) (void) memcpy( pvDestination, pvSource, uxLength )
 */
#define vPortCopyTaskName( pvDestination, pvSource, uxLength ) vPortCopyBytes( pvDestination, pvSource, uxLength )

/*
 * vPortCopyBytes copies where we do not know if bytes or words. It will
 * check if aligned and sized by words and copy by words if so (faster).
 *
 * Ports not defining this function can use:
 * #define vPortCopyBytes( pvDestination, pvSource, uxLength ) (void) memcpy( pvDestination, pvSource, uxLength )
 */
void vPortCopyBytes( void *pvDestination, const void *pvSource, unsigned portBASE_TYPE uxLength ) NAKED;


/* These macros are only here for compatibility with the test suite. */
#define portDISABLE_INTERRUPTS()	portSET_INTERRUPT_MASK()
#define portENABLE_INTERRUPTS()		portCLEAR_INTERRUPT_MASK()


/*-----------------------------------------------------------*/

/* Constants used by the test code. */
#define portTICK_RATE_MS			( ( portTickType ) 1 )

/*-----------------------------------------------------------*/

/* 
 * Types required to initialise the port layer.  This code is designed to be
 * ROMable, and therefore initialisation cannot occur at compile time.
 */

typedef void ( *portTASK_DELETE_HOOK )( void *xTask );
typedef void ( *portERROR_HOOK )(  void *xTask, signed portCHAR *pcErrorString, portBASE_TYPE xErrorCode );
typedef void ( *portIDLE_HOOK )( void );

typedef struct PORT_INIT_PARAMETERS
{
	unsigned portLONG ulCPUClockHz;
	unsigned portLONG ulTickRateHz;
	portTASK_DELETE_HOOK pxTaskDeleteHook;
	portERROR_HOOK pxErrorHook;
	portIDLE_HOOK pxIdleHook;
	unsigned portLONG *pulSystemStackLocation;
	unsigned portLONG ulSystemStackSizeBytes;
	unsigned portLONG *pulVectorTableBase;
} xPORT_INIT_PARAMETERS;

/*
 * Use in the ROMable versions to allow applications to set the address of
 * callback functions, hardware dependent parameters, etc.  This function is
 * called by vTaskInitializeScheduler() and should not be called directly by
 * application code. 
 */
void vPortInitialize( const xPORT_INIT_PARAMETERS * const pxInitParameters );


#endif /* PORTMACRO_H */

