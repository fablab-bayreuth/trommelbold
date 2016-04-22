

/*
 * Control drums via serial terminal.
 * Channels are triggered via numbers '1' to '8' (corresponding to channel + 1) and linefeed.
 * Set your terminal to 19200 baud
 */

//=============================================================================================================

#include "trommelbold.h"

//=============================================================================================================

Trommelbold drum;

//=============================================================================================================
void setup() 
{
  Serial.begin(19200);
}

//=============================================================================================================

void loop() {
  serial_tick();
  drum.tick();
}

//=============================================================================================================

void serial_tick(void)
{
  static char buffer[20] = {0};
  static uint8_t tail = 0;
  while (Serial.available())
  {
    char in = Serial.read();
    if (in != '\r' && in != '\n')  // Line termination?
    {
      // No termination, copy new character to input buffer.
      // On input buffer overflows, we just drop the surplus bytes
      if (tail < sizeof(buffer))
        buffer[tail++] = in;
    }
    else  
    {
      
      // Termination character. Evaluate received command.
      for (uint8_t i=0; i<tail; i++)
      {
        char c = buffer[i];
        if (c >= '1' && c <= '8')
        {
          int8_t chan = c - '1';
          drum.hit( chan );
          Serial.print("Trigger channel "); Serial.println(chan);
        }
      }
      tail = 0;
    }
  }
}

