
#ifndef DRUMBOLD_H
#define DRUMBOLD_H

//=============================================================================================================

#include <Arduino.h>

//=============================================================================================================

#define  N_CHAN             8    // number of attached drum channels
#define  DEF_DRUM_PORTS     {6, 7, 8, 9, 10, 11, 12, 13}
#define  DEF_BEAT_DURATION  20   // ms
#define  MAX_BEAT_DURATION  250
#define  MAX_BEATS          2    // maximum number of simultaneous beats

//=============================================================================================================

class Trommelbold
{
  public:

    uint8_t drum_ports[N_CHAN];
    uint16_t def_beat_duration;
    uint16_t max_beat_duration;
    uint8_t max_active_beats;

  protected:

    struct {
      uint8_t active;
      uint32_t ms_beat_start;
      uint32_t ms_beat_duration;
    } chan_state[N_CHAN];
    uint8_t  n_active_beats = 0;  // Keep count of total active channels
    

  public: 

//---------------------------------------------------------------------------------------------------------------

    Trommelbold() :
      drum_ports(DEF_DRUM_PORTS), def_beat_duration(DEF_BEAT_DURATION),
      max_beat_duration(MAX_BEAT_DURATION), max_active_beats(MAX_BEATS), n_active_beats(0)
      {
        // Intialize channels
        for (int ch=0; ch<N_CHAN; ch++)
        {
          digitalWrite( drum_ports[ch], 0 );
          pinMode( drum_ports[ch], OUTPUT );
          chan_state[ch].active = 1;
        } 
      
      }
      
//---------------------------------------------------------------------------------------------------------------

    void hit(uint8_t chan)  {  hit(chan, def_beat_duration);  }
      
    void hit(uint8_t chan, uint16_t ms_duration)
      {
        if (chan>=N_CHAN) return;
        digitalWrite( drum_ports[chan], 1 );
        chan_state[chan].ms_beat_start = millis();
        chan_state[chan].ms_beat_duration = min( ms_duration, max_beat_duration);
        
        if (!chan_state[chan].active)
        {
          // Channel was not active before.
          n_active_beats ++;
          while (n_active_beats > max_active_beats)
          {
            // Too many simulatneous beats. Release oldest
            uint32_t max_age = 0, now = millis();
            uint8_t  oldest = -1;
            for (uint8_t ch=0; ch<N_CHAN; ch++)
            {
              if (chan_state[ch].active)
              {
                uint32_t age = now - chan_state[ch].ms_beat_start;
                if (age > max_age)
                {
                  oldest = ch;
                  max_age = age;
                }
              }
            }
            if (oldest == -1) break;  // No active channel found. This should NOT happen normally.
            else release(oldest);
          }
      
          // Now mark new beat channel as active
          chan_state[chan].active = 1;
        }
         
      }

//---------------------------------------------------------------------------------------------------------------
      
    void release(uint8_t chan)
      {
        if (chan>=N_CHAN) return;
        digitalWrite( drum_ports[chan], 0 );
        if (chan_state[chan].active)
        {
          chan_state[chan].active = 0;
          if (n_active_beats > 0) n_active_beats --;  // Should always be true normally.  
        }
      }

//---------------------------------------------------------------------------------------------------------------

    void release_all(void)
      {
        for (int ch=0; ch<N_CHAN; ch++)
        {
          digitalWrite( drum_ports[ch], 0 );
          chan_state[ch].active = 0;
        } 
        n_active_beats = 0;
      }

//---------------------------------------------------------------------------------------------------------------
    
    void tick(void)
      {
        int32_t now = millis();
        for (uint8_t ch=0; ch<N_CHAN; ch++)
          if (chan_state[ch].active)
            if ( now - chan_state[ch].ms_beat_start >= chan_state[ch].ms_beat_duration )
              release(ch);
      }

//---------------------------------------------------------------------------------------------------------------
     
}; // End class Trommelbold

//=============================================================================================================

#endif // DRUMBOLD_H
