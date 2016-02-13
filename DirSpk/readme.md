# DirSpk Software Design

## Overview

This software runs on the [Arduino Due](https://www.arduino.cc/en/Main/ArduinoBoardDue).
The Due uses the Atmel [SAM3X8E](http://www.atmel.com/devices/sam3x8e.aspx) an
ARM Cortex-M3 based MCU. The software is built using [Atmel Studio](http://www.atmel.com/Microsite/atmel-studio/)
and Atmel's libraries, the [Atmel Software Framework](http://asf.atmel.com/docs/latest/api.html).

The bulk of the Due's time is devoted to generating PWM signals to drive the
transducer array. This operates with a high-priority interrupt. We use [FreeRTOS](http://www.freertos.org) to co-ordinate amongst lower priority
tasks such as driving the user interface.

## FreeRTOS Configuration

We are using FreeRTOS as provided through ASF. It is largely the default
configuration. Some of the more interesting
[configuration values](http://www.freertos.org/a00110.html) are:

*   `configTICK_RATE_HZ = 1000`, `configUSE_TICK_HOOK = 1`. Get a callback every
    millisecond to allow for I/O processing.  
*   `configCHECK_FOR_STACK_OVERFLOW = 1`, `configUSE_MALLOC_FAILED_HOOK = 1`. We
    don't trust our code.
*   Uses `heap_4.c` for FreeRTOS memory allocation.

## Tasks and Interrupt Routines

### 40kHz TC0 interrupt for output
Output to DAC and PWM

## I/O

(Detailed pin allocation in init.h.)

### Transducer PWM

### DAC

### ADC
Calibration and reading

### CLI
The software supports a command line interface via the UART, which is connected
to the Due's "Programming Port" via the onboard Mega16U2. This is implemented
using the ASF specific [FreeRTOS UART peripheral
](http://asf.atmel.com/docs/3.21.0/sam3x/html/group__freertos__uart__peripheral__control__group.html).

The CLI is exposed to the host USB controller as a 115200,N,8,1 serial port
connection.

### SPI pots

### SPI screen

### Encoders
