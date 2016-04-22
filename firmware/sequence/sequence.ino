
#include "sequencer.h"

//=============================================================================================================


// The drum sequence is stored as array of steps. The array length is calculated later
Step  internal_sequence[] = {
  0b00000001,
  0b00000011,
  0b00000110,
  0b00001100,
  0b10000000,
  0b11000000,
  0b01100000,
  0b00110000
};

#define  internal_bpm  120  // beats per minute


//=============================================================================================================
void setup() 
{
  Serial.begin(19200);
  sequence_start( internal_sequence, 
                  sizeof(internal_sequence)/sizeof(internal_sequence[0]),
                  internal_bpm
                );

}

//=============================================================================================================

void loop() {
  sequence_tick();
}

//=============================================================================================================


