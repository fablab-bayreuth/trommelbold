
/*
 * Control drums via MIDI.
 */

// By default, we use our own software serial implementation for midi i/o, since the version
// from the Arduino library has some severe deficiencies (eg. completly blocks while sending
// and receiving.
#define  USE_ARDUINO_SOFT_SERIAL  0  // set to 1 to use the built-in arduino software serial

//=============================================================================================================
// Midi stuff

#include <MIDI.h>  // Arduino midi library (not a standard library, must be installed

#if USE_ARDUINO_SOFT_SERIAL
    #include <SoftwareSerial.h>
    SoftwareSerial soft_serial(2,3);
    MIDI_CREATE_INSTANCE(SoftwareSerial, soft_serial, MIDI);
#else
    #include "midi_serial.h"
    MidiSoftwareSerial  midi_soft_serial;
    MIDI_CREATE_INSTANCE(MidiSoftwareSerial, midi_soft_serial, MIDI);
    // :NOTE: We can not use midi (small letters) as name for the midi-object,
    //   since the library already uses this name for the namespace
#endif

//=============================================================================================================
// Trommelbold

#include "trommelbold.h"
Trommelbold drum;

//=============================================================================================================
void setup() 
{
    // :NOTE: Hen using the built-in software serial class, it is import ant to set the hardware serial baudrate
    //   SMALLER than the software serial baudrate (31250 for midi). When using our custom software serial 
    //   implementation, this is no issue.
    Serial.begin(19200);  
    
    // Listen to MIDI on all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
    
    // :NOTE: Per default, the midi class resends every received event to the midi out port immediately 
    //  (software "midi through" function).  With the built-in software serial class, this will go horribly
    //  wrong, since it completely blocks during send, and therefore misses further input events.
    #if  USE_ARDUINO_SOFT_SERIAL
        // :NOTE: Turn Thru function off AFTER calling MIDI.begin (will be re-activated by MIDI.begin). 
        MIDI.turnThruOff();
    #else
        MIDI.turnThruOn();
    #endif
}

//=============================================================================================================

void loop() {
  drum.tick();
  midi_tick();
}

//=============================================================================================================

void midi_tick()
{
    uint8_t stat = 0, note = 0, vel = 0;
    int8_t chan = -1;
    if (MIDI.read())
    {
        stat = MIDI.getType(); 
        switch(stat)      
        {
            case midi::NoteOn:
            case midi::NoteOff:
                note = MIDI.getData1();
                vel =  MIDI.getData2();
                print_midi(stat, note, vel);
                chan = midi_note_to_drum(note);
                if (chan>=0) {
                    if (stat == midi::NoteOn)  
                         else drum.hit( chan, 20 );
                    else drum.release( chan );
                }
            break;
        }
    }
}

#define MIDI_BASE_NOTE  60-12
int8_t midi_note_to_drum(uint8_t note)
{
    switch (note)
    {
        case MIDI_BASE_NOTE +  0: return 0;
        case MIDI_BASE_NOTE +  2: return 1;
        case MIDI_BASE_NOTE +  4: return 2;
        case MIDI_BASE_NOTE +  5: return 3;
        case MIDI_BASE_NOTE +  7: return 4;
        case MIDI_BASE_NOTE +  9: return 5;
        case MIDI_BASE_NOTE + 11: return 6;
        case MIDI_BASE_NOTE + 12: return 7;
    }
    return -1;
}

    


void print_midi(uint8_t stat, uint8_t data1, uint8_t data2)
{
    Serial.print("MIDI: "); Serial.print(stat, HEX);
    Serial.print(" "); Serial.print(data1, HEX);
    Serial.print(" "); Serial.println(data2, HEX);
}


