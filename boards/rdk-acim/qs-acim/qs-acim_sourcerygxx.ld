/******************************************************************************
 *
 * qs-acim_sourcerygxx.ld - Scatter file for Sourcery CodeBench
 *
 * Copyright (c) 2007-2012 Texas Instruments Incorporated.  All rights reserved.
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
 * This is part of revision 9453 of the RDK-ACIM Firmware Package.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Define the end of the heap space, which determines the beginning of the
 * stack space.
 *
 *****************************************************************************/
__cs3_heap_end = __cs3_region_end_ram - 768;

/******************************************************************************
 *
 * Define the interrupt handlers used by the application.
 *
 *****************************************************************************/
EXTERN(NmiSR)
__cs3_isr_nmi = NmiSR;
EXTERN(FaultISR)
__cs3_isr_hard_fault = FaultISR;
EXTERN(SysTickIntHandler)
__cs3_isr_systick = SysTickIntHandler;
EXTERN(GPIOBIntHandler)
__cs3_isr_gpio_b = GPIOBIntHandler;
EXTERN(GPIOCIntHandler)
__cs3_isr_gpio_c = GPIOCIntHandler;
EXTERN(GPIODIntHandler)
__cs3_isr_gpio_d = GPIODIntHandler;
EXTERN(UART0IntHandler)
__cs3_isr_uart0 = UART0IntHandler;
EXTERN(PWM0IntHandler)
__cs3_isr_pwm0 = PWM0IntHandler;
EXTERN(MainWaveformTick)
__cs3_isr_pwm1 = MainWaveformTick;
EXTERN(MainMillisecondTick)
__cs3_isr_pwm2 = MainMillisecondTick;
EXTERN(QEIIntHandler)
__cs3_isr_qei0 = QEIIntHandler;
EXTERN(ADC0IntHandler)
__cs3_isr_adc0 = ADC0IntHandler;
