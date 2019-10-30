
#include "sequencer.h"

//=============================================================================================================

Step seq[] = {
  0b00001111,
};

#define seq_size  (sizeof(seq)/sizeof(seq[0]))

#define  bpm  240  // beats per minute

//=============================================================================================================
void setup() 
{
  Serial.begin(19200);
  pinMode( A0, INPUT_PULLUP );
  pinMode( A1, INPUT_PULLUP );
  pinMode( A2, INPUT_PULLUP );
  pinMode( A3, INPUT_PULLUP );
}

//=============================================================================================================

void loop() {
  bool input[4];
  bool last_input[4] = {1,1,1,1};
  bool play = 0;
  
  while (1) {
    input[0] = digitalRead(A0);
    input[1] = digitalRead(A1);
    input[2] = digitalRead(A3);
    input[3] = digitalRead(A2);

    play = 0;
    for (int i=0; i<4; i++) {
      if (input[i] == 0) {
        Serial.print("press: "); Serial.println(i);
        play = 1;
        if (last_input[i] != 0) {
          Serial.print("start: "); Serial.println(i);
          sequence_start( seq, seq_size, bpm*(i+1) );
        }
      }
      last_input[i] = input[i];
    }
    
    if (play)  {
      sequence_tick();
      Serial.println("tick");
    }
    else {
      sequence_stop();
    }
  }
}

//=============================================================================================================


