
/*
 * Control drums via MIDI and/or serial port.
 * 
 * On the serial port, channels are triggered via numbers '1' to '8' (corresponding to channel + 1)
 * and linefeed. Set your terminal to 19200 baud.
 */

// Enable MIDI input?
#define USE_MIDI  1

// Enable Serial input?
#define USE_SERIAL  1

#define  TROMMELBOLD_ID  "Trommelbold-v1.0"   // Device id string, can be queried via serial command "id?"

// By default, we use our own software serial implementation for MIDI i/o, since the version
// from the Arduino library has some severe deficiencies (eg. completly blocks while sending
// and receiving.
#define  USE_ARDUINO_SOFT_SERIAL  0  // set to 1 to use the built-in arduino software serial

//=============================================================================================================
// Trommelbold

#include "trommelbold.h"
Trommelbold drum;

//=============================================================================================================
// Midi stuff
#if USE_MIDI
    #include <MIDI.h>  // Arduino midi library (not a standard library, must be installed manually)
    
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
#endif

//=============================================================================================================
void setup() 
{
    #if USE_MIDI
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
    #endif

    // :NOTE: When using the built-in software serial class, it is import ant to set the hardware serial baudrate
    //   SMALLER than the software serial baudrate (31250 for midi). When using our custom software serial 
    //   implementation, this is no issue.
    Serial.begin(19200);  
    
    #if USE_SERIAL
        Serial.println("Yabadabadoo!");
    #endif
}

//=============================================================================================================

void loop() {
    #if USE_MIDI
        midi_tick();
    #endif
    #if USE_SERIAL
        serial_tick();
    #endif
    
    drum.tick();
}

//=============================================================================================================
// MIDI handling

#if USE_MIDI

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
                             drum.hit( chan, 20 );
                        else drum.release( chan );
                    }
                break;
            }
        }
    }
    
    #define MIDI_BASE_NOTE  60
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

#endif  // USE_MIDI


//=============================================================================================================
// Serial handling

#if USE_SERIAL

    void serial_tick(void)
    {
        static char cmd_buffer[20] = {0};   // Buffer for incoming command strings
        static uint8_t tail = 0;     // Next free position in buffer
        while (Serial.available())
        {
            char in = Serial.read();
            if (in != '\r' && in != '\n')  // line termination?
            {
                // Copy new character to input buffer.
                // On input buffer overflows, we just drop further bytes
                if (tail < sizeof(cmd_buffer))
                    cmd_buffer[tail++] = in;
            }
            else  // Received termination character. Evaluate received command.
            {
                cmd_buffer[tail] = '\0';
                cmd_buffer[sizeof(cmd_buffer)-1] = '\0';   // just to be sure
                eval_serial_cmd( strlwr(cmd_buffer) );
                tail = 0;  // reset receive buffer
            }
        }
    }
    
    
    void eval_serial_cmd( const char* cmd )
    {
        if (!strncmp( cmd, "trommelbold?", 9 ))
        {
            Serial.println("YESSIR!");
        }
        
        else if (!strncmp( cmd, "id?", 3 ))
        {
            Serial.println(TROMMELBOLD_ID);
        }
        
        else if (!strncmp( cmd, "mute", 4 )
              || !strncmp( cmd, "mute", 3 ) )
        {
            drum.release_all();
        }
        
        else  // drum command
        {
            char c;
            while (c = *cmd++)
            {
                if (c >= '1' && c <= '8')  { // plain number --> hit channel
                    drum.hit( c - '1' );
                }
                else if (c=='h' || c=='r') {
                    int chan = atoi( cmd );
                    if (chan > 0) {
                        if (c=='h') drum.hit(chan-1);      // h<i> --> hit channel i
                        if (c=='r') drum.release(chan-1);  // r<i> --> release channel i
                    }
                    while (c = *cmd) {  // skip number
                        if (isdigit(c)) cmd++;
                        else break;
                    }
                }
            }
        }
    }

#endif  // USE_SERIAL



