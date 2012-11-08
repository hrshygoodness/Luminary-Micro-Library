; <<< Use Configuration Wizard in Context Menu >>>
;******************************************************************************
;
; startup_rvmdk.S - Startup code for use with Keil's uVision.
;
; Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
; Software License Agreement
; 
; Texas Instruments (TI) is supplying this software for use solely and
; exclusively on TI's microcontroller products. The software is owned by
; TI and/or its suppliers, and is protected under applicable copyright
; laws. You may not combine this software with "viral" open-source
; software in order to form a larger program.
; 
; THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
; NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
; NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
; A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
; CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
; DAMAGES, FOR ANY REASON WHATSOEVER.
; 
; This is part of revision 9453 of the RDK-BLDC Firmware Package.
;
;******************************************************************************

;******************************************************************************
;
; <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
;
;******************************************************************************
Stack   EQU     0x00001000

;******************************************************************************
;
; <o> Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
;
;******************************************************************************
Heap    EQU     0x00000000

;******************************************************************************
;
; Allocate space for the stack.
;
;******************************************************************************
        AREA    STACK, NOINIT, READWRITE, ALIGN=3
StackMem
        SPACE   Stack
__initial_sp

;******************************************************************************
;
; Allocate space for the heap.
;
;******************************************************************************
        AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
HeapMem
        SPACE   Heap
__heap_limit

;******************************************************************************
;
; Indicate that the code in this file preserves 8-byte alignment of the stack.
;
;******************************************************************************
        PRESERVE8

;******************************************************************************
;
; Place code into the reset code section.
;
;******************************************************************************
        AREA    RESET, CODE, READONLY
        THUMB

;******************************************************************************
;
; External declarations for the interrupt handlers used by the application.
;
;******************************************************************************
        EXTERN  IntDefaultHandler
        EXTERN  ADC0IntHandler
        EXTERN  GPIOBIntHandler
        EXTERN  GPIOCIntHandler
        EXTERN  FaultISR
        EXTERN  NmiSR
        EXTERN  PWM0IntHandler
        EXTERN  MainWaveformTick
        EXTERN  MainMillisecondTick
        EXTERN  QEIIntHandler
        EXTERN  SysTickIntHandler
        EXTERN  Timer0AIntHandler
        EXTERN  Timer1AIntHandler
        EXTERN  WatchdogIntHandler

;******************************************************************************
;
; The vector table.
;
;******************************************************************************
        EXPORT  __Vectors
__Vectors
        DCD     StackMem + Stack            ; Top of Stack
        DCD     Reset_Handler               ; Reset Handler
        DCD     NmiSR                       ; NMI Handler
        DCD     FaultISR                    ; Hard Fault Handler
        DCD     IntDefaultHandler           ; The MPU fault handler
        DCD     IntDefaultHandler           ; The bus fault handler
        DCD     IntDefaultHandler           ; The usage fault handler
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     0                           ; Reserved
        DCD     IntDefaultHandler           ; SVCall handler
        DCD     IntDefaultHandler           ; Debug monitor handler
        DCD     0                           ; Reserved
        DCD     IntDefaultHandler           ; The PendSV handler
        DCD     SysTickIntHandler           ; The SysTick handler
        DCD     IntDefaultHandler           ; GPIO Port A
        DCD     GPIOBIntHandler             ; GPIO Port B
        DCD     GPIOCIntHandler             ; GPIO Port C
        DCD     IntDefaultHandler           ; GPIO Port D
        DCD     IntDefaultHandler           ; GPIO Port E
        DCD     IntDefaultHandler           ; UART0 Rx and Tx
        DCD     IntDefaultHandler           ; UART1 Rx and Tx
        DCD     IntDefaultHandler           ; SSI0 Rx and Tx
        DCD     IntDefaultHandler           ; I2C0 Master and Slave
        DCD     IntDefaultHandler           ; PWM Fault
        DCD     PWM0IntHandler              ; PWM Generator 0
        DCD     MainWaveformTick            ; PWM Generator 1
        DCD     MainMillisecondTick         ; PWM Generator 2
        DCD     QEIIntHandler               ; Quadrature Encoder 0
        DCD     ADC0IntHandler              ; ADC Sequence 0
        DCD     IntDefaultHandler           ; ADC Sequence 1
        DCD     IntDefaultHandler           ; ADC Sequence 2
        DCD     IntDefaultHandler           ; ADC Sequence 3
        DCD     WatchdogIntHandler          ; Watchdog timer
        DCD     Timer0AIntHandler           ; Timer 0 subtimer A
        DCD     IntDefaultHandler           ; Timer 0 subtimer B
        DCD     Timer1AIntHandler           ; Timer 1 subtimer A
        DCD     IntDefaultHandler           ; Timer 1 subtimer B
        DCD     IntDefaultHandler           ; Timer 2 subtimer A
        DCD     IntDefaultHandler           ; Timer 2 subtimer B
        DCD     IntDefaultHandler           ; Analog Comparator 0
        DCD     IntDefaultHandler           ; Analog Comparator 1
        DCD     IntDefaultHandler           ; Analog Comparator 2
        DCD     IntDefaultHandler           ; System Control (PLL, OSC, BO)
        DCD     IntDefaultHandler           ; FLASH Control
        DCD     IntDefaultHandler           ; GPIO Port F
        DCD     IntDefaultHandler           ; GPIO Port G
        DCD     IntDefaultHandler           ; GPIO Port H
        DCD     IntDefaultHandler           ; UART2 Rx and Tx
        DCD     IntDefaultHandler           ; SSI1 Rx and Tx
        DCD     IntDefaultHandler           ; Timer 3 subtimer A
        DCD     IntDefaultHandler           ; Timer 3 subtimer B
        DCD     IntDefaultHandler           ; I2C1 Master and Slave
        DCD     IntDefaultHandler           ; Quadrature Encoder 1
        DCD     IntDefaultHandler           ; CAN0
        DCD     IntDefaultHandler           ; CAN1
        DCD     IntDefaultHandler           ; CAN2
        DCD     IntDefaultHandler           ; Ethernet
        DCD     IntDefaultHandler           ; Hibernate

;******************************************************************************
;
; This is the code that gets called when the processor first starts execution
; following a reset event.
;
;******************************************************************************
        EXPORT  Reset_Handler
Reset_Handler
        ;
        ; Call the C library enty point that handles startup.  This will copy
        ; the .data section initializers from flash to SRAM and zero fill the
        ; .bss section.
        ;
        IMPORT  __main
        B       __main

;******************************************************************************
;
; Make sure the end of this section is aligned.
;
;******************************************************************************
        ALIGN

;******************************************************************************
;
; Some code in the normal code section for initializing the heap and stack.
;
;******************************************************************************
        AREA    |.text|, CODE, READONLY

;******************************************************************************
;
; The function expected of the C library startup code for defining the stack
; and heap memory locations.  For the C library version of the startup code,
; provide this function so that the C library initialization code can find out
; the location of the stack and heap.
;
;******************************************************************************
    IF :DEF: __MICROLIB
        EXPORT  __initial_sp
        EXPORT  __heap_base
        EXPORT  __heap_limit
    ELSE
        IMPORT  __use_two_region_memory
        EXPORT  __user_initial_stackheap
__user_initial_stackheap
        LDR     R0, =HeapMem
        LDR     R1, =(StackMem + Stack)
        LDR     R2, =(HeapMem + Heap)
        LDR     R3, =StackMem
        BX      LR
    ENDIF

;******************************************************************************
;
; Make sure the end of this section is aligned.
;
;******************************************************************************
        ALIGN

;******************************************************************************
;
; Tell the assembler that we're done.
;
;******************************************************************************
        END
