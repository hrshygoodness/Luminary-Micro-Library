/******************************************************************************
 *
 * timers_sourcerygxx.ld - Scatter file for Sourcery CodeBench
 *
 * Copyright (c) 2012 Texas Instruments Incorporated.  All rights reserved.
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
 * This is part of revision 9453 of the EK-LM4F120XL Firmware Package.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Define the end of the heap space, which determines the beginning of the
 * stack space.
 *
 *****************************************************************************/
__cs3_heap_end = __cs3_region_end_ram - 256;

/******************************************************************************
 *
 * Define the interrupt handlers used by the application.
 *
 *****************************************************************************/
EXTERN(Timer0IntHandler)
__cs3_isr_timer0a = Timer0IntHandler;
EXTERN(Timer1IntHandler)
__cs3_isr_timer1a = Timer1IntHandler;
