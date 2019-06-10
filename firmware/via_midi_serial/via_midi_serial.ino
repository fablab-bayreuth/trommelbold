/* 
 * Control drums via MIDI and/or serial port.
 * 
 * Serial port:
 *   19200 baud, ascii format, terminate with linefeed
 *   Numbers 1-8 directly trigger drum channels 1-8
 *   h<i>/r<i>  trigger/release channel i-SERIAL_BASE_NOTE
 *   mute       release all drum channels
 *   id?        returns id-string
 * 
 * MIDI:
 *   Note-On/Off <i> events trigger drum channels i-MIDI_BASE_NOTE
 */
 
// ==========================================================================================================
 
// MIDI input
#define USE_MIDI   1             
#define RECV_MIDI  1             // Play drums via MIDI
#define MIDI_IN_TO_OUT   1       // Forward MIDI in events to MIDI out (software through) 
#define MIDI_BASE_NOTE   60      // Midi note corresponding to first drum channel
#define MIDI_WHITE_KEYS_ONLY  1  // Use only white keys on piano keyboard (dur scale)


// Serial input
#define RECV_SERIAL  1           // Play drums via serial port
#define SERIAL_TO_MIDI    1      // Forward serial events to MIDI out

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
// Midi stuff. Only include if needed.

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
    if (RECV_MIDI)  
    {
        MIDI.begin(MIDI_CHANNEL_OMNI);    // Listen to MIDI on all channels

        // :NOTE: Per default, the midi class resends every received event to the midi out port immediately 
        //  (software "midi through" function).  With the built-in software serial class, this will go horribly
        //  wrong, since it completely blocks during send, and therefore misses further input events.
        if (USE_ARDUINO_SOFT_SERIAL)
            // :NOTE: Turn Thru function off AFTER calling MIDI.begin (will be re-activated by MIDI.begin). 
            MIDI.turnThruOff();
        else if(MIDI_IN_TO_OUT)
            MIDI.turnThruOn();
    }
    
    // :NOTE: When using the built-in software serial class, it is import ant to set the hardware serial baudrate
    //   SMALLER than the software serial baudrate (31250 for midi). When using our custom software serial 
    //   implementation, this is no issue.
    Serial.begin(19200);  
    Serial.println("Yabadabadoo!");
}

//=============================================================================================================

void loop()
{
    #if RECV_MIDI
        midi_tick();
    #endif
    #if RECV_SERIAL
        serial_tick();
    #endif
    
    drum.tick();
}

//=============================================================================================================
// MIDI handling. Only include if switched on.

#if USE_MIDI

uint8_t  midi_base_note = MIDI_BASE_NOTE;
uint8_t midi_clock;
uint8_t midi_beat_16;
uint8_t midi_beat_4;
uint8_t midi_beat_1;

void midi_tick()
{
    uint8_t stat = 0, note = 0, vel = 0;
    int8_t chan = -1;
    if (MIDI.read())  {
        stat = MIDI.getType(); 
        switch(stat)  {
            // Note messages
            case midi::NoteOn:
            case midi::NoteOff:
                note = MIDI.getData1();
                vel =  MIDI.getData2();
                print_midi(stat, note, vel);
                chan = midi_note_to_drum(note);
                if (RECV_MIDI  &&  chan >= 0)  {
                    if (stat == midi::NoteOn)  
                         drum.hit( chan, 20 );
                    else drum.release( chan );
                }
                break;
                
              // Realtime messages
              case midi::Stop:
                  Serial.println("MIDI: Stop");
                  break;  
              case midi::Start:
                  Serial.println("MIDI: Start");
                  midi_clock = midi_beat_16 = midi_beat_4 = midi_beat_1 = 0;
                  if (midi_clock == 0)  { drum.hit( 1, 20 ); drum.hit( 0, 20 ); }
                  break;
              case midi::Continue:
                  Serial.println("MIDI: Continue");
                  if (midi_clock == 0)  { drum.hit( 1, 20 ); drum.hit( 0, 20 ); }
                  break;
              case midi::Clock:
                  if (++midi_clock == 6)  {  // 6 clocks per 1/16 beat
                      midi_clock = 0;
                      if (++midi_beat_16 == 4)  {
                          midi_beat_16 = 0;
                          drum.hit( 0, 20 );
                          Serial.print("MIDI: beat_4 "); Serial.println(midi_beat_4);
                          if (++midi_beat_4 == 4)  {
                              midi_beat_4 = 0;
                              ++midi_beat_1;
                              drum.hit( 1, 20 );
                          }
                      }                      
                  }
                  break;
        }
    }
}

#if MIDI_WHITE_KEYS_ONLY
  const int8_t midi_note_to_drum_tab[] = {0,-1,1,-1,2,3,-1,4,-1,5,-1,6,7};
  const int8_t drum_to_midi_note_tab[] = {0,2,4,5,7,9,11,12,-1,-1,-1,-1,-1};
#else
  const int8_t midi_note_to_drum_tab[] = 70,1,2,3,4,5,6,7,-1,-1,-1,-1,-1};
  const int8_t drum_to_midi_note_tab[] = {0,1,2,3,4,5,6,7,-1,-1,-1,-1,-1};
#endif

int8_t midi_note_to_drum(uint8_t note)
{
    if (note < midi_base_note)
        return -1;
    note -= midi_base_note;
    if (note > sizeof(midi_note_to_drum_tab))
        return -1;
    int8_t drum = midi_note_to_drum_tab[note];   
    return drum;
}
    
int8_t drum_to_midi_note(uint8_t drum)
{
    if (drum > sizeof(drum_to_midi_note_tab))
        return -1;
    int8_t note = drum_to_midi_note_tab[drum];
    if (note >= 0)  return note + midi_base_note;
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

void serial_tick(void)
{
    static char cmd_buffer[20] = {0};   // Buffer for incoming command strings
    static uint8_t tail = 0;     // Next free position in buffer
    while (Serial.available())
    {
        char in = Serial.read();
        if (in != '\r'  &&  in != '\n')  // line termination?
        {
            // Copy new character to input buffer.
            // On input buffer overflows, we just drop further bytes
            if (tail < sizeof(cmd_buffer))
                cmd_buffer[tail++] = in;
        }
        else  // Received termination character. Evaluate command.
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
    if (!strncmp( cmd, "trommelbold?", 9 ))  {
        Serial.println("YESSIR!");
    }
    else if (!strncmp( cmd, "id?", 3 ))  {
        Serial.println(TROMMELBOLD_ID);
    }
    else if (!strncmp( cmd, "mute", 4 )
          || !strncmp( cmd, "mute", 3 ) )  {
        drum.release_all();
    }
    else  // drum command
    {
        char c;
        while (c = *cmd++)
        {
            if (c >= '1' && c <= '8') {  // plain number --> hit channel
                int8_t chan = c - '0';
                drum.hit( chan-1 );      
                if (USE_MIDI  &&  SERIAL_TO_MIDI) {
                    int8_t note = drum_to_midi_note(chan - 1);
                    Serial.print("Fwd: "); Serial.print("chan: "); Serial.print(chan);
                    Serial.print(" note: "); Serial.println(note);
                    if (note >= 0)
                        MIDI.send( midi::NoteOn, note, 64, 1 );
                }
            }
            else if (c=='h' || c=='r')  {
                int chan = atoi( cmd );   // returns 0 if not a number
                if (chan > 0)  {   
                    if (c=='h')  {   // h<i> --> hit channel i
                        drum.hit( chan-1 );      
                        if (USE_MIDI  &&  SERIAL_TO_MIDI) {
                            int8_t note = drum_to_midi_note(chan - 1);
                            Serial.print("Fwd: "); Serial.print("chan: "); Serial.print(chan);
                            Serial.print(" note: "); Serial.println(note);
                            if (note >= 0)
                                MIDI.send( midi::NoteOn, note, 64, 1 );
                        }
                    }
                    else if (c=='r')  {  // r<i> --> release channel i
                        drum.release( chan-1 );  
                        if (USE_MIDI  &&  SERIAL_TO_MIDI)  {
                            int8_t note = drum_to_midi_note(chan - 1);
                            if (note >= 0)
                              MIDI.send( midi::NoteOff, note, 64, 1 );
                        }   
                    } 
                }
                while (c = *cmd) {  // skip number
                    if (isdigit(c)) cmd++;
                    else break;
                }
            }
        }
    }
}

extern "C" void dout( const char* msg, int i)
{
    Serial.print(msg); Serial.println(i, HEX);
}
