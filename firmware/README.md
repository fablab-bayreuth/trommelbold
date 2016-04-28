Trommelbold Arduino firmware

Drum control is encapsuled in class Trommelbold in file trommelbold.h
At the moment, this file is included in all sketch directories, (until
I find out how to reference other diretories from within an Arduino sketch.)


test
----
Just a simple test that succesively triggers all channels to test the hardware.


sequence
--------
Play sequences stored in the firmware.


via_serial
----------
Control the drum channels from the PC via the usb-serial connection
(19200 baud, LF or CR line termination). Send "1"-"8" plus line termination
to trigger a channel.


via_midi
--------
Control the drum channels via the MIDI interface. Channels 1-8 are triggered
by MIDI notes 60,62,64,65,67,69,71,71 (one octave in the middle of my MIDI keyboard,
white keys only). Solenoid on time is controlled by the key velocity.

**NOTE**: 
In the current version, the user can switch between the built-in software serial 
class from the Arduino library and out own, more reliably software serial 
implementation. (The built-in version blocks the system while sending and receiving.)
This firmware uses the external Arduino Midi library 
(http://playground.arduino.cc/Main/MIDILibrary),
which needs to be installed separately. This dependency might be dropped in future
versions.


