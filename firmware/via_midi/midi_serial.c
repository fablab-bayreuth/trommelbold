
//====================================================================================================

/*
 * A simple oftware serial port.
 * Data is received on Arduino pin 2 (RD2), which is also the external INT0 interrupt pin.
 * The baudrate is fixed to 31250 baud (=1Mhz/32) for MIDI.
 * The implementation uses the INT0 interrupt and the timer2 output compare match interrupts.
 * ==> PWM on pins 3 and 11 (those are controlled by timer 2) and the attachInterrupt() function 
 *     (uses the INT0 interrupt) can not be used.
 */
 
//====================================================================================================

#include "Arduino.h"
#include "midi_serial.h"

//====================================================================================================

// Receive machine. Using INT0 interrupt to detect start bits and
//   Timer 2 compare A interrupt for timing.

#define  RX_DEBUG  0   // When set, timing edges are generatedd on PD3 at the times when the input is sampled

// :NOTE: We use an enum type as incrementable state here. This is only allowed in pure C, not in C++
static volatile enum {  
    RX_OFF = 0, RX_IDLE, RX_START, RX_DATA_0, RX_DATA_1, RX_DATA_2,
    RX_DATA_3, RX_DATA_4, RX_DATA_5, RX_DATA_6, RX_DATA_7, RX_STOP 
  }  rx_state = RX_OFF;

#define  RX_QUEUE_SIZE  32
static volatile uint8_t  rx_queue[RX_QUEUE_SIZE];
static volatile uint8_t  rx_queue_put, rx_queue_get, rx_queue_n;
static volatile uint8_t  rx_reg;
static volatile uint8_t  rx_drop_next;

//----------------------------------------------------------------------------------------------------

void midi_rx_init(void)
{
  // :NOTE: Race condition aput. The two employed ISRs may enable each others interrupt.
  cli();                                // :NOTE: cli will disable all interrupts, even when occuring simultaneously (unlike on the PIC8 platform).
  EIMSK &= ~(1<<INT0) & ~(1<<INT1);     // disable INTx interrupts
  TIMSK2 &= ~(1<<OCIE2A);               // disable compare A interrupt
  sei();
  
  #if RX_DEBUG   // DEBUG: strobe pin to measure timing
    DDRD |= (1<<3);  // PD3 as output
  #endif

  DDRD &= ~(1<<2);  // PD2 as input
    
  rx_reg = 0;
  rx_queue_put = 0;
  rx_queue_get = 0;
  rx_queue_n = 0;
  rx_drop_next = 0;

  rx_state = RX_IDLE;   // initial state: wait for silence on the RX line

  // Timer 2: 
  TCCR2A = 0b00000000;   // WGM2 = 000: normal mode
  TCCR2B = 0b00000011;   // WGM22=0, CS2=011: prescaler=32
  OCR2A = TCNT2 + 176;   // compare A: 11 bit times in the future

  EICRA = 0b00000010;    // ISC0=10: INT0 triggers on falling edge
  EIFR = (1<<INTF0);     // clear INT0 interrupt flag 
                         // :NOTE: On this platform, interrupt bits are cleared by wriing a ONE to the respective bit.
                         //   Compare the Atmel FAQ: http://www.atmel.com/webdoc/AVRLibcReferenceManual/FAQ_1faq_intbits.html
  EIMSK |= (1<<INT0);    // enable INT0 interrupt
}

//----------------------------------------------------------------------------------------------------

// The INT0 ISR will be triggered by a falling edge on the INT0 pin (eg. a start bit of a serial byte)
ISR( INT0_vect )
{
  #if RX_DEBUG   // DEBUG: toggle pin to measure timing
      PIND = (1<<3);  // :NOTE: Writing a one to PINx (the port input registers) toggles the output state
  #endif
  
  // after init, wait for at least 11 bits of silence on the serial line, to make sure we are in sync 
  if (rx_state == RX_IDLE)
    if (!(TIFR2 & (1<<OCF2A)))  // has compare A interrupt already triggered (11 bit times after init)
    {                           // if not,
      OCR2A = TCNT2 + 176;      //   then reset compare A to 11 bit times in the future and return
      return;
    }
    
  // else we assume this is a start bit
  rx_reg = 0;
  rx_state = RX_DATA_0;  // wait for first data bit

  OCR2A = TCNT2 + 24 - 3;  // trigger compare A at 1.5 bit times (- measured latency) in the future
                           //   --> TOV2 will trigger in the middle of the 1st data bit
                           
  EIMSK = 0;             // disable INT0 / INT1 interrupt
  TIFR2 = (1<<OCF2A);     // clear compare A interrupt flag (:NOTE: see above)
  TIMSK2 |= (1<<OCIE2A);  // enable compare A interrupt 

}

//----------------------------------------------------------------------------------------------------

ISR( TIMER2_COMPA_vect )
{ 
  if (rx_state < RX_STOP)  // waiting for next data bit
  {    
    OCR2A += 16;   // +1 bit time
    #if RX_DEBUG   // DEBUG: strobe pin to measure timing
      PIND = (1<<3);
    #endif
    if (PIND & (1<<2))  rx_reg = (rx_reg>>1) | 0b10000000;
    else  rx_reg >>= 1;
    rx_state ++;
  }
  else // waiting for stop bit
  {
    #if RX_DEBUG   // DEBUG: strobe pin to measure timing
      PIND = (1<<3);
    #endif
    if (PIND & (1<<2))  // must be high for stop bit
    {
      // Store received byte in queue
      if (rx_drop_next)
        rx_drop_next = 0;
      else if (rx_queue_n < RX_QUEUE_SIZE)  // on overflow: fail silently by dropping latest byte
      {
        rx_queue[rx_queue_put++] = rx_reg;
        if (rx_queue_put >= RX_QUEUE_SIZE)  rx_queue_put = 0;
        rx_queue_n++;
      }

      TIMSK2 &= ~(1<<OCIE2A);  // disable compare A interrupt
      EIFR = (1<<INTF0);       // clear INT0 interrupt flags
      EIMSK = 0b00000001;      // enable INT0 interrupt
      rx_state = RX_START;     // state: waiting for next start bit
    }
    else  // framing error_ fail silently
    {
      rx_state = RX_IDLE;     // reset to idle state to recover sync
      OCR2A = TCNT2 + 176;    // 11 bit times in the future
    }
  }
}

//----------------------------------------------------------------------------------------------------

int midi_rx_available(void)
{
  return rx_queue_n;
}

//----------------------------------------------------------------------------------------------------

int midi_rx_read(void)
{
  if (rx_queue_n > 0)
  {
    uint8_t data = rx_queue[rx_queue_get++];
    if (rx_queue_get >= RX_QUEUE_SIZE)  rx_queue_get = 0;
    cli();  // race condition aput: decrement is non-atomic on this platform, rx_queue_n may be written in isr
    rx_queue_n --;
    sei();
    return data;
  }
  else // queue empty
    return -1;
}

//----------------------------------------------------------------------------------------------------

int midi_rx_peek(void)
{
  if (rx_queue > 0)
    return rx_queue[rx_queue_get];
  else  // queue empty
    return -1;
}

//----------------------------------------------------------------------------------------------------

void midi_rx_clear(void)
{
  rx_queue_get = 0;
  cli();         // :NOTE: Race condition ahead. rx_queue_put and rx_queue_n are written from the ISR
  rx_queue_put = 0;
  rx_queue_n = 0;
  rx_drop_next = 1;
  sei();
}

  
//=====================================================================================================

// Transmit machine. Using Timer 2 compare B interrupt for timing.

static volatile enum {
      TX_OFF = 0, TX_IDLE, TX_START, TX_DATA_0, TX_DATA_1, TX_DATA_2,
      TX_DATA_3, TX_DATA_4, TX_DATA_5, TX_DATA_6, TX_DATA_7, TX_STOP
  } tx_state = TX_OFF;

#define  TX_QUEUE_SIZE  32
static volatile uint8_t  tx_queue[TX_QUEUE_SIZE];
static volatile uint8_t  tx_queue_put, tx_queue_get, tx_queue_n;
static volatile uint8_t  tx_reg;

//----------------------------------------------------------------------------------------------------

void midi_tx_init(void)
{
  EIMSK &= ~(1<<INT1);      // disable INT1 interrupt (shared by PD3 pin)
  TIMSK2 &= ~(1<<OCIE2B);   // disable timer 2 compare B interrupt
  
  PORTD |= (1<<3);   // ensure high
  DDRD |= (1<<3);    // PD3 as output
  PORTD |= (1<<3);   // idle high
  
  tx_state = TX_IDLE;   // nothing to do yet
  tx_reg = 0;
  tx_queue_put = 0;
  tx_queue_get = 0;
  tx_queue_n = 0;

  // Timer 2:
  TCCR2A = 0b00000000;   // WGM2 = 000: normal mode
  TCCR2B = 0b00000011;   // WGM22=0, CS2=011: prescaler=32

  OCR2B = TCNT2 + 2;   // :NOTE: It seems the interrupt flags can not be set manually. Instead, trigger it by setting a compare
                       //   point in the near future. We use 2 ticks here, since it is unclear what happens with the prescaler.

  // do not enable the tx interrupt here. It will be enabled in the tx_write function.
}

//----------------------------------------------------------------------------------------------------

void midi_tx_write(uint8_t data)
{ 
  if (tx_state == TX_OFF)  midi_tx_init();

  // Store received byte in queue
  if (tx_queue_n >= TX_QUEUE_SIZE)  return;   // on overflow: silently drop this byte
  tx_queue[tx_queue_put++] = data;
  if (tx_queue_put >= TX_QUEUE_SIZE)  tx_queue_put = 0;
  
  cli();          // :NOTE: Race condition ahead: tx_queue_n may be decremented in ISR, increment operation is non-atomic on this platform
  tx_queue_n++;
  TIMSK2 |= (1<<OCIE2B); // enable tx interrupt. :NOTE: Do not touch the timer compare value here
  sei();
}

//----------------------------------------------------------------------------------------------------

ISR( TIMER2_COMPB_vect )
{ 
  if (tx_state == TX_IDLE || tx_state >= TX_STOP)  // idle or just finished a stop bit
  {
    if (tx_queue_n > 0)  // Output data pending?
    {
      // apply start bit
      PORTD &= ~(1<<3);      // start bit is always clear
      OCR2B = TCNT2 + 16 - 3;    // trigger again in 1 bit time (- measured latency)
      tx_state = TX_START;
      tx_reg = tx_queue[tx_queue_get++];
      if (tx_queue_get >= TX_QUEUE_SIZE)  tx_queue_get = 0;
      tx_queue_n --;
      return;
    }
    else  // nothing to send
    {
      // stay idle and disable tx interrupt
      TIMSK2 &= ~(1<<OCIE2B);   // disable compare B interrupt
      OCR2B = TCNT2 + 2;        // make sure we trigger upon next activation of the interrupt
      return;
    }
  }
  else if (tx_state < TX_STOP)  // next bit to be sent
  {
    OCR2B += 16;  // next interrupt at +1 bit time
    if ((tx_state == TX_DATA_7) || (tx_reg & 1))  // after bit 7 send the stop bit, which is always set
      PORTD |= (1<<3);
    else  PORTD &= ~(1<<3);
    tx_reg >>= 1;
    tx_state ++;
    return;
  }
}

//----------------------------------------------------------------------------------------------------

// Number of pending output bytes
int midi_tx_pending(void)  
{
  return tx_queue_n;
}

//----------------------------------------------------------------------------------------------------

// Number of free space in transmit queue
int midi_tx_available(void)
{
  return TX_QUEUE_SIZE - tx_queue_n;
}

//----------------------------------------------------------------------------------------------------

void midi_tx_clear(void)
{
  // :NOTE: Ongoing transmission will not be cancelled
  tx_queue_put = 0;
  sei();
  tx_queue_get = 0;
  tx_queue_n = 0;
  cli();
}

//----------------------------------------------------------------------------------------------------

void midi_tx_flush(void)
{
  while (tx_queue_n > 0)
    ; // wait
}

//=====================================================================================================
