

/*
 * Control drums via MIDI.
 */

//=============================================================================================================

#include <SoftwareSerial.h>
SoftwareSerial soft_serial(2,3);

#include <MIDI.h>
MIDI_CREATE_INSTANCE(SoftwareSerial, soft_serial, MIDI);  // do not use midi (small letters) as object name
                                                          //   (is already used as namespace name)
#include "trommelbold.h"
Trommelbold drum;


//=============================================================================================================


//=============================================================================================================
void setup() 
{
  // :NOTE: Set the hardware serial baudrate SMALLER than the software serial baudrate (31250 for midi)  
  Serial.begin(19200);  

  // Listen to MIDI on all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);

  // :NOTE: Per default, the midi class copies everything to modi out. This will go horribly wrong with
  //   SoftwareSerial interface whose write function is BLOCKING! 
  // Turn Thru off AFTER calling begin (will be re-activated by begin). 
  MIDI.turnThruOff();    
}

//=============================================================================================================

void loop() {
  drum.tick();
  midi_tick();
}

//=============================================================================================================

void midi_tick()
{
  uint8_t type = 0, note = 0, vel = 0;
  int8_t chan = -1;
  if (MIDI.read())
  {
      type = MIDI.getType(); 
      switch(type)      
      {
          case midi::NoteOn:
          case midi::NoteOff:
              note = MIDI.getData1();
              vel =  MIDI.getData2();
              print_midi(type, note, vel);
              chan = midi_note_to_drum(note);
              if (chan>=0) {
                if (type == midi::NoteOn)  
                     drum.hit( chan, vel );
                else drum.release( chan );
              }
          break;
       }
  }
}


int8_t midi_note_to_drum(uint8_t note)
{
  switch (note)
  {
    case 60: return 0;  // = C on middle octave
    case 62: return 1;
    case 64: return 2;
    case 65: return 3;
    case 67: return 4;
    case 69: return 5;
    case 71: return 6;
    case 72: return 7;
  }
  return -1;
}

    


void print_midil(uint8_t stat, uint8_t data1, uint8_t data2)
{
  Serial.print("MIDI: ");
  Serial.print(to_hex((stat>>4)&0x0f));
  Serial.print(to_hex(stat&0x0f));
  Serial.print(" ");
  Serial.print(to_hex((data1>>4)&0x0f));
  Serial.print(to_hex(data1&0x0f));
  Serial.print(" ");
  Serial.print(to_hex((data2>>4)&0x0f));
  Serial.print(to_hex(data1&0x0f));
  Serial.println("");
}

char to_hex(uint8_t in)
{
  if (in>16) return ' ';
  if (in<10) return '0'+in;
  else return 'A'+in-10;
}

