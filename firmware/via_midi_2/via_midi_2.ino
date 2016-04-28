

/*
 * Under development.
 * Midi with a custom software serial implementation with non-blocking read/write.
 */

//============================================================================

#include "midi_serial.h"
MidiSoftwareSerial  midi_soft_serial;

#include <MIDI.h>
MIDI_CREATE_INSTANCE(MidiSoftwareSerial, midi_soft_serial, MIDI);  // do not use midi (small letters) as object name

//============================================================================

void setup() {
  Serial.begin(19200);
  midi_soft_serial.begin();

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();  // midi through should work non-blocking now
}


void loop() {

#if 0 // test send
  while (1)
  {
    int i = 0;
    if (midi_tx_available())
    {
      midi_tx_write(0b01010101);
      midi_tx_write(i);
      i++;
      delay(5);
    }
  }
#endif

#if 1  // test receive
  while (midi_rx_available())
  {
    uint8_t in = midi_rx_read();
    if (midi_tx_available)
      midi_tx_write(in);
    Serial.print("RX: ");
    Serial.println(in, HEX);
  }
#endif

}



//=====================================================================================================

