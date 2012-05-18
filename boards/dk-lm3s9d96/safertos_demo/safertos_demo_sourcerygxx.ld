/******************************************************************************
 *
 * safertos_demo_sourcerygxx.ld - Scatter file for Sourcery CodeBench
 *
 * Copyright (c) 2009-2012 Texas Instruments Incorporated.  All rights reserved.
 * Software License Agreement
 * 
 * Texas Instruments (TI) is supplying this software for use solely and
 * exclusively on TI's microcontroller products. The software is owned by
 * TI and/or its suppliers, and is protected under applicable copyright
 * laws. You may not combine this software with "viral" open-source
 * software in order to form a larger program.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
 * NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
 * NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
 * CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES, FOR ANY REASON WHATSOEVER.
 * 
 * This is part of revision 8555 of the DK-LM3S9D96 Firmware Package.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Define the end of the heap space, which determines the beginning of the
 * stack space.
 *
 *****************************************************************************/
__cs3_heap_end = __cs3_region_end_ram - 512;

/******************************************************************************
 *
 * The location of the SafeRTOS vectors in ROM.  These are derived from
 * SafeRTOS/SafeRTOS_API.h since the GNU linker can not handle C include files.
 *
 *****************************************************************************/
vSafeRTOS_SVC_Handler_Address = 0x02000f45;
vSafeRTOS_PendSV_Handler_Address = 0x02000fdd;
vSafeRTOS_SysTick_Handler_Address = 0x02001175;

/******************************************************************************
 *
 * Define the interrupt handlers used by the application.
 *
 *****************************************************************************/
__cs3_isr_svcall = vSafeRTOS_SVC_Handler_Address;
__cs3_isr_pendsv = vSafeRTOS_PendSV_Handler_Address;
__cs3_isr_systick = vSafeRTOS_SysTick_Handler_Address;
EXTERN(TouchScreenIntHandler)
__cs3_isr_adc3 = TouchScreenIntHandler;
EXTERN(lwIPEthernetIntHandler)
__cs3_isr_ethernet0 = lwIPEthernetIntHandler;
