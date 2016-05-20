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


via_midi_serial
---------------
Control the drum channels via the MIDI interface, or from the PC via the 
usb-serial connection. Witthe MIDI input, drum channels 1-8 are triggered
by MIDI notes 60,62,64,65,67,69,71,71 (one octave in the middle of my MIDI
keyboard, white keys only). Solenoid on time is controlled by the key velocity.
With the usb-serial connection, send "1"-"8" to trigger a channel.
(use 19200 baud, LF or CR line termination). 

**NOTE**: 
In the current MIDI implementation, the user can switch between the built-in
software serial class from the Arduino library and out own, more reliably
software serial implementation. (The built-in version blocks the system while
sending and receiving.) The firmware uses the external Arduino Midi library 
(http://playground.arduino.cc/Main/MIDILibrary), which needs to be installed
manually. This dependency might be dropped in future versions.


via_serial
----------
(Obsolete, use via_midi_serial instead)
Control the drum channels from the PC via the usb-serial connection
(19200 baud, LF or CR line termination). Send "1"-"8" plus line termination
to trigger a channel.


via_midi
--------
(Obsolete, use via_midi_serial instead)
Control the drum channels via the MIDI interface. Channels 1-8 are triggered
by MIDI notes 60,62,64,65,67,69,71,71 (one octave in the middle of my MIDI keyboard,
white keys only). Solenoid on time is controlled by the key velocity.

