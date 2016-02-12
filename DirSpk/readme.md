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

*   `configTICK_RATE_HZ = 1000`. FreeRTOS ticks at 1000Hz. This is quite fast,
    and we may lower it in future.  
*   Uses `heap_4.c` for FreeRTOS memory allocation.
