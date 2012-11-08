#******************************************************************************
#
# Makefile - Rules for building the dk-lm3s9d96 examples.
#
# Copyright (c) 2008-2012 Texas Instruments Incorporated.  All rights reserved.
# Software License Agreement
# 
# Texas Instruments (TI) is supplying this software for use solely and
# exclusively on TI's microcontroller products. The software is owned by
# TI and/or its suppliers, and is protected under applicable copyright
# laws. You may not combine this software with "viral" open-source
# software in order to form a larger program.
# 
# THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
# NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
# NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
# CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
# DAMAGES, FOR ANY REASON WHATSOEVER.
# 
# This is part of revision 9453 of the DK-LM3S9D96 Firmware Package.
#
#******************************************************************************

#
# A list of the directories containing the examples.
#
DIRS=bitband           \
     blinky            \
     boot_demo1        \
     boot_demo2        \
     boot_demo_eth     \
     boot_demo_usb     \
     boot_eth_ext      \
     boot_eth          \
     boot_serial       \
     boot_usb          \
     calibrate         \
     dbeeprom          \
     enet_io           \
     enet_lwip         \
     enet_ptpd         \
     enet_uip          \
     ext_demo_1        \
     ext_demo_2        \
     fontview          \
     gpio_jtag         \
     grlib_demo        \
     hello             \
     hello_widget      \
     i2s_demo          \
     i2s_filter        \
     i2s_speex_enc     \
     IQmath_demo       \
     interrupts        \
     lang_demo         \
     mpu_fault         \
     otg_detect        \
     qs-checkout       \
     safertos_demo     \
     scribble          \
     sd_card           \
     showjpeg          \
     softuart_echo     \
     timers            \
     uart_echo         \
     udma_demo         \
     usb_dev_audio     \
     usb_dev_bulk      \
     usb_dev_caudiohid \
     usb_dev_keyboard  \
     usb_dev_msc       \
     usb_dev_mouse     \
     usb_dev_serial    \
     usb_host_audio    \
     usb_host_audioin  \
     usb_host_keyboard \
     usb_host_msc      \
     usb_stick_demo    \
     usb_stick_update  \
     videocap          \
     watchdog         

#
# The default rule, which causes the examples to be built.
#
all:
	@for i in ${DIRS};              \
	 do                             \
	     make -C $${i} || exit $$?; \
	 done

#
# The rule to clean out all the build products.
#
clean:
	@rm -rf ${wildcard *~} __dummy__
	@-for i in ${DIRS};        \
	  do                       \
	      make -C $${i} clean; \
	  done
