
#--------------------------------------------------------------------------
# Settings
#--------------------------------------------------------------------------

fullscreen = 0
W, H = 1200, 700  # will be overriden by fullscreen
window_caption = 'Sequencer'

bpm = 120  # Beats per minute

# Key matrix
n_steps = 16
n_channels = 8

# MIDI settings
play_midi = 1
midi_channel = 10  # gm drumset
midi_notes = [ 52, 44, 42, 80, 70, 69, 38, 36 ]  # see gm drum sound table
midi_device = 1  # On my Macbook: Internal wavetable synth

#Trommelbold
play_drumbold = 1
drumbold_port = 'COM6'

# Audio output
play_click = 0

# Desired repetition interval for main loop
main_dt = 0.05  


#--------------------------------------------------------------------------
# Modules
#--------------------------------------------------------------------------

import time, os, sys
from numpy import *

if sys.platform == 'win32': timer = time.clock  # on windows, clock() is the high precision wall time
else: timer = time.time  # on linux, clock() is the cpu time, while time() is the high accuray wall time

#--------------------------------------------------------------------------
# Pygame
#--------------------------------------------------------------------------

import pygame
os.environ['SDL_VIDEO_CENTERED'] = '1'
pygame.mixer.pre_init( buffer=32)
pygame.init()
if fullscreen:
    screen = pygame.display.set_mode( (0,0), pygame.FULLSCREEN )    
    W = pygame.display.Info().current_w
    H = pygame.display.Info().current_h
else:
    screen = pygame.display.set_mode( (W,H) )
    
pygame.display.set_caption(window_caption)
screen.fill((0,0,0))
pygame.display.flip()

font_normal = pygame.font.SysFont('default', 30)


# -----------------------------------------------------------------------------
# Midi
# -----------------------------------------------------------------------------

if play_midi:
    import pygame.midi
    pygame.midi.init()
    if midi_device != None:
        midi_device = pygame.midi.get_default_output_id()
    midi_out = pygame.midi.Output(midi_device)
Note_On = lambda key, vel: (0x90+((midi_channel-1)&0x0f), key&0x7f, vel&0x7f)
Program_Change = lambda prog: (0xC0+((midi_channel-1)&0x0f), prog&0x7f )

##midi_program = 119
##midi_out.write_short( *Program_Change(midi_program-1) )


# -----------------------------------------------------------------------------
# Trommelbold via serial
# -----------------------------------------------------------------------------

if play_drumbold:
    import serial
    drumbold_ser = serial.Serial( drumbold_port, baudrate=19200 )

def send_drumbold( chan ):
    if play_drumbold:
        drumbold_ser.write( str(chan+1)+'\r' )


# -----------------------------------------------------------------------------
# Audio
# -----------------------------------------------------------------------------

tick = pygame.mixer.Sound('snd/tick.wav')
tack = pygame.mixer.Sound('snd/tack.wav')


#--------------------------------------------------------------------------
# Gui widgets
#--------------------------------------------------------------------------

# Window structure
H1 = 100   # Top of key matrix. Widgets above

from pgu import gui as pgui
gui = pgui.App()
gui_cnt = pgui.Container(width=W, height=H)

# ---- BPM-slider ----------------------------------

def on_slider_bpm(slider_bpm):
    global bpm
    slider_bpm_label.set_text( "%d BPM" % int(slider_bpm.value) )
    bpm = int(slider_bpm.value)

slider_bpm_label = pgui.Label("%d BPM" %bpm , font=font_normal, color=(230,230,230))
slider_bpm = pgui.HSlider(value=120, min=30, max=360, size=32, width=300, height=20 )
slider_bpm.connect(pgui.CHANGE, on_slider_bpm, slider_bpm)
gui_cnt.add(slider_bpm_label, W-30-300, 20)
gui_cnt.add(slider_bpm,       W-30-300, 50)


# ---- MIDI-Out select box ----------------------------------
##
##def on_select_midi(select_midi):
##    global midi_out
##    print 'Select midi device:', select_midi.value
##    if play_midi:
##        midi_out.close()
##        midi_out = pygame.midi.Output( select_midi.value )
##    
##def select_midi_fill(select_midi, crop=1):
##    if not play_midi:
##        select_midi.add( 'Disabled', 0 )
##        return
##    for id in range( pygame.midi.get_count() ):
##        intf, name, inp, outp, op = pygame.midi.get_device_info(id)
##        if outp:
##            select_midi.add( '%d: %s' % ( id, name.replace('Microsoft','')[:13] ),
##                             id )
##
##select_midi_label = pgui.Label("MIDI-Out" , font=font_normal, color=(230,230,230))
##select_midi = pgui.Select(value=midi_device, width=180, height=20 )
##select_midi_fill(select_midi)
##select_midi.connect(pgui.CHANGE, on_select_midi, select_midi)
##gui_cnt.add(select_midi_label, W-30-510, 20)
##gui_cnt.add(select_midi,       W-30-510, 50)

# ---- Run/Stop-Button ------------------------------

def on_button_run(button_run):
    global seq_run
    if seq_run: seq_run = 0; button_run.value = 'Play'
    else: seq_run = 1; button_run.value = 'Stop'
    
button_run = pgui.Button('Play', width=60, height=50)
button_run.connect(pgui.CLICK, on_button_run, button_run)
gui_cnt.add( button_run, 30, 25 )

# ---- Clear-Button ------------------------------

def on_button_clear(button_clear):
    key_matrix.set_all(0)
    
button_clear = pgui.Button('Clear', width=60, height=50)
button_clear.connect(pgui.CLICK, on_button_clear, button_clear)
gui_cnt.add( button_clear, 120, 25 )


# ---- Load-Button ------------------------------

button_save = pgui.Button('Load', width=60, height=20)
##button_save.connect(pgui.CLICK, on_button_save, button_save)
gui_cnt.add( button_save, 225, 55 )


# ---- Save-Button ------------------------------

def on_button_save(button_save):
    d = pgui.FileDialog()
    ##d.connect(gui.CHANGE, handle_file_browser_closed, d)
    d.open()
    
button_save = pgui.Button('Save', width=60, height=20)
##button_save.connect(pgui.CLICK, on_button_save, button_save)
gui_cnt.add( button_save, 310, 55 )


# ---- Filename-Textbox ------------------------------

box_filename = pgui.Input(value="Hello there", width = 160, height = 18)
gui_cnt.add( box_filename, 220, 25 )


# ---- Bank select-Radio buttons ------------------------------

radio_bank_label = pgui.Label("Bank" , font=font_normal, color=(230,230,230))
group_bank = pgui.Group(name='bank_select', value=0)
table_bank = pgui.Table(width=200, height=20)
table_bank.tr()
for i in range(10):
    if i%5 == 0:
        table_bank.td( pgui.Spacer(width=10, height=20) )
    table_bank.td( pgui.Radio(group_bank, i) )
    

gui_cnt.add( radio_bank_label, 420, 25 )
gui_cnt.add( table_bank, 420, 55 )



##filename
##bank


gui.init( gui_cnt, screen )

# -----------------------------------------------------------------------------
# Keyboard matrix
# -----------------------------------------------------------------------------

from keymatrix import KeyMatrix

key_matrix = KeyMatrix( n_steps, n_channels )
##key_matrix.place( screen, screen.get_rect() )
key_matrix.place( screen, (0, H1, W, H-H1) )


# -----------------------------------------------------------------------------
# Load startup sequence
# -----------------------------------------------------------------------------

try: key_matrix.set_matrix( loadtxt('sequence.dat', dtype=int) )
except: print 'Error loading sequence'

# -----------------------------------------------------------------------------
# Main loop
# -----------------------------------------------------------------------------

main_run = 1           # Set to 0 in main loop to terminate program
main_t_next = None     # Timestamp of next main loop iteration

seq_run = 0         # Set to 1 to start sequencer, set to 0 to stop sequencer.
seq_running = 0     # Flags that the sequencer is actually running. Do NOT modify maually!

seq_step = None   # Current beat step in sequence
seq_t_next = None # Timestamp of next beat in sequence

try:
    
    while main_run:
        
        # Pygame events
        for event in pygame.event.get():
            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_q or \
                   event.key == pygame.K_ESCAPE:
                    main_run = 0
                elif event.key == pygame.K_SPACE:
                    if seq_run: seq_run = 0
                    else: seq_run = 1
                elif event.key == pygame.K_s:
                    print 'Save sequence'
                    savetxt('sequence.dat', key_matrix.get_matrix(), fmt='%d')
                elif event.key == pygame.K_l:
                    print 'Load sequence' 
                    try: key_matrix.set_matrix( loadtxt('sequence.dat', dtype=int) )
                    except: print 'Error loading sequence'
                elif event.key == pygame.K_c:
                    key_matrix.set_all(0)
            elif event.type == pygame.QUIT:
                main_run = 0
            elif event.type == pygame.MOUSEMOTION:
                pass
            elif event.type == pygame.MOUSEBUTTONDOWN:
                ##print 'Mouse down at', event.pos
                key = key_matrix.click(event.pos)
                if key:
                    midi_out.write_short( *Note_On(midi_notes[key.channel], 127) )
            elif event.type == pygame.MOUSEBUTTONUP:
                ##print 'Mouse up at', event.pos
                pass

            # pass event to gui
            gui.event(event)

        if not main_run: break


        # Sequence
        if seq_run and not seq_running:  # Sequencer is to be started
            seq_running = 1
            seq_t_next = timer()
            seq_step = -1

        if not seq_run and seq_running:  # Sequencer is to be stopped
            seq_running = 0
            
        if seq_running and timer() >= seq_t_next:
            seq_step += 1
            seq_step %= n_steps
            print seq_step, '  +%d ms' % int(1e3*(timer()-seq_t_next))
            seq_t_next +=  60./bpm  # time of next beat

            if play_click:
                if seq_step%8==0: tick.play()
                else: tack.play()
            
            if play_midi:
                # For a drum set, we only send Note On events
                for key in key_matrix.get_keys(seq_step):
                    if key.active:
                        print 'Play', midi_notes[key.channel]
                        midi_out.write_short( *Note_On(midi_notes[key.channel], 127) )

            # Trommelbold
            for n,key in enumerate( key_matrix.get_keys(seq_step) ):
                if key.active:
                    send_drumbold(n) 
                        
        # Screen
        screen.fill((0,0,0))
        key_matrix.draw(seq_step)
        gui.paint(screen)
        pygame.display.flip()


        # Main loop clock
        if main_t_next == None: main_t_next = timer()
        main_t_next += main_dt
        while timer() < main_t_next:
            if timer()<main_t_next-0.02: time.sleep(0.001)
            else: pass
                

                


finally:
    pygame.quit()
    if play_midi:
        midi_out.close()
        pygame.midi.quit()
        # XXX: Throws an error when closing. Seems to be a bug in pygames portmidi wrapper
