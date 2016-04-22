
#ifndef SEQUENCER_H
#define SEQUENCER_H

//=============================================================================================================

#include "trommelbold.h"

//-------------------------------------------------------------------------------------------------------------

Trommelbold drum;

//-------------------------------------------------------------------------------------------------------------

// Store sequence in flash
// For 8 drum channels per step, we use he bits of a single byte
typedef  const PROGMEM uint8_t  Step;

#define  DEF_BPM  240
#define  DEF_REPEAT  1

//-------------------------------------------------------------------------------------------------------------

bool sequence_run = 0;
bool sequence_repeat = 1;
Step* current_sequence = 0;
size_t sequence_length = 0;
size_t next_pos = 0;
uint32_t  last_time = 0;
int32_t micros_per_step = 0;

//-------------------------------------------------------------------------------------------------------------

void sequence_tick(bool force_step=0)
{
  if (!sequence_run)  return;
  if (force_step) last_time = micros();

  // Is it time for the next step?
  if (sequence_run)
    if ( micros()-last_time >= micros_per_step
         || force_step )
    {
      if (next_pos >= sequence_length)
      {
        if (!sequence_repeat)
        {
          sequence_run = 0;
          return;
        }
        else next_pos = 0;
      }
  
      Serial.print("Step"); Serial.println(next_pos);
      
      // Apply next step in sequence.
      uint8_t s = pgm_read_byte( &current_sequence[next_pos] );
      for (uint8_t ch=0; ch<8; ch++)
      {
        if (s&1) drum.hit(ch);
        s >>= 1;   
      }
      next_pos ++;
      if (!force_step) last_time += micros_per_step;
      // Check position overflow at next step to maintain timing
    }

  // Perform drum tick (independently of sequence steps!)
  drum.tick();
}

//-------------------------------------------------------------------------------------------------------------

void sequence_start(Step* sequence, size_t length, float bpm=DEF_BPM, bool repeat=DEF_REPEAT )
{
  current_sequence = sequence;
  sequence_length = length;
  next_pos = 0;
  micros_per_step = 1e6 * 60 / bpm;
  sequence_repeat = repeat;
  sequence_run = 1;
  sequence_tick(1);  // force first step immediately
}


//=============================================================================================================

#endif // SEQUENCER_H
