
/*
 *  The MidiSoftwareSerial class is a C++ wrapper or the midi_serial software serial implementation.
 *  It provides an interface that is code-compaible to Arduino's Serial an SoftwarSerial classes,
 *  and thus can be used with the Arduino MIDI library.
 */

#include "midi_serial.h"
#include <stddef.h>

//==============================================================================================

class MidiSoftwareSerial
{

  public:
    
    MidiSoftwareSerial() { }   // nothing to do here
    
    void begin(unsigned long ignored = 0)
      {
        bool is_initiated = 0;  // :NOTE: will be shared by all instances. Prevents multible calls to init
        if (!is_initiated)
        {
          midi_rx_init();
          midi_tx_init();
          is_initiated = 1;
        }
      }
      
    int available(void)  { return midi_rx_available(); }
    
    int peek(void)  { return midi_rx_peek(); }
    
    int read(void)  { return midi_rx_read(); }
    
    int availableForWrite(void)  { return midi_tx_available(); }
    
    void flush(void)  { midi_tx_flush(); };
    
    size_t write(uint8_t data)  
      { 
        midi_tx_write(data); 
        return 1; 
      }
};

//==============================================================================================

