
#ifndef MIDI_SERIAL_H
#define MIDI_SERIAL_H
#ifdef __cplusplus
  extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

//====================================================================================================

/*  A simple software serial port.
 *  Data is received on Arduino pin 2 (RD2), which is also the external INT0 interrupt pin.
 *  The baudrate is fixed to 31250 baud (=1Mhz/32) for MIDI.
 *  The implementation uses the INT0 interrupt and the timer 2 output compare match interrupts.
 *  ==> PWM on pins 3 and 11 (those are controlled by timer 2 and the attachInterrupt() function
 *      (uses the INT0 interrupt) can not be used.
 */

//---------------------------------------------------------------------------------------------------
// Public functions

void midi_rx_init(void);         // Initialize receive engine.
int  midi_rx_available(void);    // Returns the number of bytes in receive buffer.
int  midi_rx_read(void);         // Read and remove the next byte from the receive buffer. Returns -1 if the receive buffer is empty.
int  midi_rx_peek(void);         // Get the next byte from the receive buffer, without removing it. Returns -1 if the receive buffer is empty.
void midi_rx_clear(void);        // Clears the receive buffer and any partly received data.

void midi_tx_init(void);            // Initialize transmit engine
void midi_tx_write(uint8_t data);   // Write one byte to the transmit buffer. If the buffer is currently empty, the transmission will start immediately.
int  midi_tx_pending(void);         // Returns the number of bytes waiting in transmit buffer.
int  midi_tx_available(void);       // Returns the free space in the transmit queue.
void midi_tx_clear(void);           // Clears the transmit buffers. Ongoing transmissions will not be canceled.
void midi_tx_flush(void);           // Waits until the ouput buffer has been emptied

//=====================================================================================================

#ifdef __cplusplus
  }  // extern "C"

//==============================================================================================

/*  MidiSoftwareSerial class is just a C++ wrapper or the midi_serial software serial implementation.
 *  It provides an interface that is code-compatible to Arduino's Serial an SoftwareSerial classes,
 *  and thus can be used with the Arduino MIDI library.
 */

//---------------------------------------------------------------------------------------------------

class MidiSoftwareSerial
{
  public:
    MidiSoftwareSerial() { }   // nothing to do here  
    int available(void)  { return midi_rx_available(); }
    int peek(void)  { return midi_rx_peek(); }
    int read(void)  { return midi_rx_read(); }
    int availableForWrite(void)  { return midi_tx_available(); }
    void flush(void)  { midi_tx_flush(); };
    
    void begin(unsigned long ignored = 0)
      {
        static bool is_initiated = 0;  // :NOTE: will be shared by all instances. Prevents multible calls to init
        if (!is_initiated)
        {
          midi_rx_init();
          midi_tx_init();
          is_initiated = 1;
        }
      }
 
    size_t write(uint8_t data)  
      { 
        midi_tx_write(data); 
        return 1; 
      }
};

//==============================================================================================
  
#endif // __cplusplus
#endif // MIDI_SERIAL_H









