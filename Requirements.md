# Directional Speaker 1

## v0.1

### Purpose
The purpose of this version is to verify the basic principles of operation:

1.  demonstrate that Arduino Due is capable of handling the data rate, in and
    out.
2.  demonstrate accuracy in reading digital signal
3.  prototype software for the box
4.  characterize the electrical properties
5.  characterize the audio properties

### Requirements

*   Operation from power supply
*   Breadboard/wires OK. Finish not required.
*   User interface via Serial Port
*   Single channel audio in via 3/8" plug
*   Audio channel out
    *   40 kHz PWM to test devices
    *   DAC output to Speaker

### Hardware Design

#### Power

*   12v provided by bench power supply
    *   directly fed to 40kHz output circuitry
    *   also power Due

#### Preamp

*   Uses LM324 quad op-amp powered by 3.3v sourced from Due
*   50k pot + 500 ohm resistor provides 100:1 gain range

#### DAC output

*   DAC output across whole range of DAC (0.55v - 2.86v) mirroring input
*   Output amplified by one stage of LM324 3.3v op amp
*   Audible via emitter follower amplifier

#### Transducer output
*   40kHz PWM output
*   Via half-bridge drivers and N-channel MOSFETs
*   Mechanism to measure current
*   Will want to experiment with inductor.


### Sofware Design

#### On-chip Peripherals and pins

*   Timer/Clock
    * 40kHz interrupt
    * 1kHz main loop timer
*   ADC
    * Arudino pin X? =
*   DAC
    * Arduino pin x?
*   PWM
    * Arduino pin x?

#### Initialization
*   Peripheral set up
*   Set bridge driver pins low. Commence 40kHz signal

#### 40kHz Timer
*   Read previous ADC value
*   Depending on mode
    *   Output to DAC
    *   Output to PWM


#### Main Loop
*   Wait for 1kHz timer
*   If serial input available
    *   Display menu
*   If DAC mode requested
    *   Turn on DAC. Set DAC mode bit.
*   If 40kHz requested
    *   Enable PWM. Set 40kHz mode bit.
*   If data output requested
    *   Show highest reading in past 5 100ms samples.

## Future

*   Measure frequency from real time clock
*   Native serial output (why?)
*   Measure output current
*   In-product output current measurement
*   Use PC Power supply
*   Two input channels
*   50 transducers
*   100 transducers
*   Two output channels
*   Display
*   Buttons
