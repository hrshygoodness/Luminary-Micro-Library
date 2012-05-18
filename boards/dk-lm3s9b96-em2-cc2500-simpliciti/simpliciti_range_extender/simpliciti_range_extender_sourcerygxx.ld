/******************************************************************************
 *
 * simpliciti_range_extender_sourcerygxx.ld - Scatter file for Sourcery CodeBench
 *
 * Copyright (c) 2010-2012 Texas Instruments Incorporated.  All rights reserved.
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
 * This is part of revision 8555 of the DK-LM3S9B96-EM2-CC2500-SIMPLICITI Firmware Package.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Define the end of the heap space, which determines the beginning of the
 * stack space.
 *
 *****************************************************************************/
__cs3_heap_end = __cs3_region_end_ram - 1024;

/******************************************************************************
 *
 * Define the interrupt handlers used by the application.
 *
 *****************************************************************************/
EXTERN(SysTickHandler)
__cs3_isr_systick = SysTickHandler;
EXTERN(BSP_GpioPort1Isr)
__cs3_isr_gpio_c = BSP_GpioPort1Isr;
EXTERN(TouchScreenIntHandler)
__cs3_isr_adc3 = TouchScreenIntHandler;
EXTERN(BSP_GpioPort1Isr)
__cs3_isr_gpio_g = BSP_GpioPort1Isr;
EXTERN(BSP_GpioPort1Isr)
__cs3_isr_gpio_h = BSP_GpioPort1Isr;
