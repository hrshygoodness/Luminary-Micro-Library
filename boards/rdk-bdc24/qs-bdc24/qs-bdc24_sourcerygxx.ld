/******************************************************************************
 *
 * qs-bdc24_sourcerygxx.ld - Scatter file for Sourcery CodeBench
 *
 * Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
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
 * This is part of revision 8555 of the RDK-BDC24 Firmware Package.
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
 * Define the interrupt handlers used by the application.
 *
 *****************************************************************************/
EXTERN(ServoIFIntHandler)
__cs3_isr_gpio_a = ServoIFIntHandler;
EXTERN(EncoderIntHandler)
__cs3_isr_gpio_c = EncoderIntHandler;
EXTERN(UART0IntHandler)
__cs3_isr_uart0 = UART0IntHandler;
EXTERN(ControllerIntHandler)
__cs3_isr_pwm2 = ControllerIntHandler;
EXTERN(ADCIntHandler)
__cs3_isr_adc0 = ADCIntHandler;
EXTERN(WatchdogIntHandler)
__cs3_isr_watchdog = WatchdogIntHandler;
EXTERN(CAN0IntHandler)
__cs3_isr_can0 = CAN0IntHandler;
