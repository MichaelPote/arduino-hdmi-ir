# Arduino HDMI CEC to AV Reciever Bridge

This is an Arduino project which links my non-CEC Onkyo AV Reciever to my LG SmartTV which has CEC / ARC over HDMI so that I can control the reciever from my LG's tv remote.

The project uses the [Arduino CEC Library](https://github.com/lucadentella/ArduinoLib_CEClient) based on [the work of Florian Echtler](https://github.com/floe/CEC)

The IR portion was adapted from [Arduino-IRRemote](https://github.com/Arduino-IRremote).

## Project Description

So this is a relatively straight forward combination of using the CEClient library to read audio commands and information from the TV and then using an IR LED to transmit the corresponding 
remote control signals to the reciever to react to. 

**This code is only designed for the Arduino Uno**.

### IR Transmission

At first I tried just using the two libraries (CEC and IRRemote) together but quickly found that the `delay()` commands used by the IR sending code would cause the CEC decoding to stop and 
miss any packets being sent. So what I did was use two of the Arduino's timers to build an interrupt driven IR transmitter.

A very helpful resource for the NEC IR Protocol used was https://www.sbprojects.net/knowledge/ir/nec.php

In order to build a non-blocking, interrupt driven IR transmitter, I used Timer 1 and Timer 2 on the Arduino Uno.
Timer 1 is being used to generate the 38khz PWM carrier signal on pin 9 that the IR system turns on and off to encode the IR commands.
Timer 2 is being used to turn on and off the PWM wave at very precise times in order to encode the NEC protocol.

The code uses a circular buffer of NEC protocol commands so the HDMI CEC portion enqueues commands for the IR portion to send at a later stage.

### Circuit 

The circuit design is exactly the example provided in the CEC repository:

![SchematicCEC](https://raw.githubusercontent.com/MichaelPote/arduino-hdmi-ir/master/extras/CECIn.png)

And the following addition was used for the IR sending:

![SchematicIR](https://raw.githubusercontent.com/MichaelPote/arduino-hdmi-ir/master/extras/IROut.png)


