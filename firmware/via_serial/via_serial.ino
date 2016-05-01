

/*
 * Control drums via serial terminal.
 * Channels are triggered via numbers '1' to '8' (corresponding to channel + 1) and linefeed.
 * Set your terminal to 19200 baud.
 */
//=============================================================================================================

#define  TROMMELBOLD_ID  "Trommelbold-v1.0"   // Device id string

//=============================================================================================================

#include "trommelbold.h"

//=============================================================================================================

Trommelbold drum;

//=============================================================================================================
void setup() 
{
    Serial.begin(19200);
    Serial.println("Yabadabadoo!");
}

//=============================================================================================================

void loop() {
    serial_tick();
    drum.tick();
}

//=============================================================================================================

void serial_tick(void)
{
    static char cmd_buffer[20] = {0};   // Buffer for incoming command strings
    static uint8_t tail = 0;     // Next free position in buffer
    while (Serial.available())
    {
        char in = Serial.read();
        if (in != '\r' && in != '\n')  // line termination?
        {
            // Copy new character to input buffer.
            // On input buffer overflows, we just drop further bytes
            if (tail < sizeof(cmd_buffer))
                cmd_buffer[tail++] = in;
        }
        else  // Received termination character. Evaluate received command.
        {
            cmd_buffer[tail] = '\0';
            cmd_buffer[sizeof(cmd_buffer)-1] = '\0';   // just to be sure
            eval_serial_cmd( strlwr(cmd_buffer) );
            tail = 0;  // reset receive buffer
        }
    }
}


void eval_serial_cmd( const char* cmd )
{
    if (!strncmp( cmd, "trommelbold?", 9 ))
    {
        Serial.println("YESSIR!");
    }
    
    else if (!strncmp( cmd, "id?", 3 ))
    {
        Serial.println(TROMMELBOLD_ID);
    }
    
    else if (!strncmp( cmd, "mute", 4 )
          || !strncmp( cmd, "mute", 3 ) )
    {
        drum.release_all();
    }
    
    else  // drum command
    {
        char c;
        while (c = *cmd++)
        {
            if (c >= '1' && c <= '8')  { // plain number --> hit channel
                drum.hit( c - '1' );
            }
            else if (c=='h' || c=='r') {
                int chan = atoi( cmd );
                if (chan > 0) {
                    if (c=='h') drum.hit(chan-1);      // h<i> --> hit channel i
                    if (c=='r') drum.release(chan-1);  // r<i> --> release channel i
                }
                while (c = *cmd) {  // skip number
                    if (isdigit(c)) cmd++;
                    else break;
                }
            }
        }
    }
}

