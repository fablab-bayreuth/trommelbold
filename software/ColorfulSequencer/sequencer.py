145
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
enable_midi = 1   # can be disabled if your system does not support the api
play_midi = 1
midi_channel = 10  # gm drumset
shift_notes = 0
##midi_notes = [ 52, 44, 42, 80, 70, 69, 38, 36 ]  # see gm drum sound table
##midi_notes = [ 60-12, 62-12, 64-12, 65-12, 67-12, 69-12, 71-12, 72-12,
##               60+12, 62+12, 64+12, 65+12, 67+12, 69+12, 71+12, 72+12 ]
midi_notes = [ 60, 62, 64, 65, 67, 69, 71, 72,  60+12, 62+12, 64+12, 65+12, 67+12, 69+12, 71+12, 72+12 ]
midi_notes = [n + shift_notes for n in midi_notes]
midi_def_device = 1  # On my Macbook: Internal wavetable synth

# Trommelbold via serial port
play_trbold = 1
trbold_def_port = 'COM6'

# Audio output
play_click = 0

# Desired repetition interval for main loop
main_dt = 0.005


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
pygame.mixer.pre_init( buffer=32 )
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

if enable_midi:
    import pygame.midi
    pygame.midi.init()
    midi_out = None

    def list_midi_devices():
        devices = []   # Enumerate output devices 
        for id in range(pygame.midi.get_count()):
            intf, name, inp, outp, op = pygame.midi.get_device_info(id)
            if outp: devices.append([name,id])
        return devices
            
    midi_devices = list_midi_devices()        
    if midi_def_device in [id for (name,id) in midi_devices]:
        midi_out = pygame.midi.Output(midi_def_device)

    def midi_out_is_open():
        try:
            return pygame.midi.get_device_info(midi_out.device_id)[4]
        except: return False

    Note_On = lambda key, vel: (0x90+((midi_channel-1)&0x0f), key&0x7f, vel&0x7f)
    Program_Change = lambda prog: (0xC0+((midi_channel-1)&0x0f), prog&0x7f )

    def send_midi( chan ):
        if play_midi and midi_out_is_open():
            midi_out.write_short( *Note_On(midi_notes[chan], 127) )



# -----------------------------------------------------------------------------
# Trommelbold via serial
# -----------------------------------------------------------------------------

import trbold_com
trbold = trbold_com.TrommelboldCom()
trbold_ports = trbold_com.list_ports()
if trbold_def_port in trbold_ports:
    trbold.open( trbold_def_port )

def send_trbold( chans ):
    if not chans: return
    if play_trbold and trbold.is_open():
            trbold.hit( chans )


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

# The pgui.Select widget lacks a handy clear method
def select_clear(self): self.values = []; self.options.clear()   
pgui.Select.clear = select_clear


# ---- BPM-slider ----------------------------------

def on_slider_bpm(_widget):
    global bpm
    slider_bpm_label.set_text( "%d BPM" % int(_widget.value) )
    bpm = int(_widget.value)

slider_bpm_label = pgui.Label("%d BPM" %bpm , font=font_normal, color=(230,230,230))
slider_bpm = pgui.HSlider(value=120, min=30, max=900, size=32, width=300, height=20 )
slider_bpm.connect(pgui.CHANGE, on_slider_bpm)
gui_cnt.add(slider_bpm_label, W-30-300, 20)
gui_cnt.add(slider_bpm,       W-30-300, 50)


# ---- MIDI-Out select box ----------------------------------
if enable_midi:
    def on_select_midi(_widget):
        global midi_out
        seq_stop()
        if _widget.value != 'None':
            print 'Select midi output:', _widget.value
            if midi_out_is_open(): midi_out.close()
            midi_out = pygame.midi.Output( _widget.value )
        else:
            print 'Close midi output'
            midi_out.close()
        
    def select_midi_fill(select_midi, crop=1):
        select_midi.clear()
        select_midi.add('--None--','None')  # make sure there is at least one entry
        ##for (name, id) in midi_devices:      # uses midi device lst assembled at program start
        for (name, id) in list_midi_devices():  # uses currently available devices
            select_midi.add( str(id)+': '+name.replace('Microsoft ','')[:13], id)
        if midi_out_is_open():
            select_midi.value = midi_out.device_id
        else: select_midi.value = 'None'
                            
    select_midi_label = pgui.Label("MIDI" , font=font_normal, color=(230,230,230))
    select_midi = pgui.Select(width=180, height=20 )
    select_midi_fill(select_midi)
    select_midi.connect(pgui.CHANGE, on_select_midi)

    def on_switch_midi(_widget):
        global play_midi
        if (_widget.value):
            print 'Play MIDI on'
            play_midi = 1
        else:
            print 'Play MIDI off'
            play_midi = 0
    switch_midi = pgui.Switch()
    switch_midi.connect(pgui.CHANGE, on_switch_midi)
    if play_midi: switch_midi.value = 1

    gui_cnt.add(select_midi_label, W-30-630, 25+3)
    gui_cnt.add(switch_midi,       W-30-555, 25+3)
    gui_cnt.add(select_midi,       W-30-530, 25)


# ---- Trommelbold serial out select box ------------------------------
def on_select_trbold_change(_widget):
    seq_stop()
    if _widget.value != 'None':
        print 'Select Trommelbold on port', _widget.value
        trbold.open(_widget.value)
    else:
        print 'Close Trommelbold'
        trbold.close()
        
def select_trbold_fill(select_trbold):
    select_trbold.clear()
    select_trbold.add('--None--','None')  # make sure there is at least one entry
    ##for p in trbold_ports: select_trbold.add(p,p)  # uses trbold list assembled at program start
    for p in trbold_com.list_ports():   # uses currently available ports
        p = str(p)
        select_trbold.add(p,p) 
    if trbold.is_open():
        select_trbold.value = trbold.portname
    else: select_trbold.value = 'None'
    
select_trbold_label = pgui.Label("T-Bold" , font=font_normal, color=(230,230,230))
select_trbold = pgui.Select(width=180, height=20 )
select_trbold_fill( select_trbold )
select_trbold.connect(pgui.CHANGE, on_select_trbold_change)

def on_switch_trbold_change(_widget):
    global play_trbold
    if (_widget.value):
        print 'Play Trommelbold on'
        play_trbold = 1
    else:
        print 'Play Trommelbold off'
        play_trbold = 0
switch_trbold = pgui.Switch()
switch_trbold.connect(pgui.CHANGE, on_switch_trbold_change)
if play_trbold: switch_trbold.value = 1

gui_cnt.add(select_trbold_label, W-30-630, 55+3)
gui_cnt.add(switch_trbold,       W-30-555, 55+3)
gui_cnt.add(select_trbold,       W-30-530, 55)


# ---- Run/Stop-Button ------------------------------

def on_button_run(_widget):
    global seq_run
    if seq_run: seq_stop()
    else:       seq_start()
    
button_run = pgui.Button('Play', width=60, height=50)
button_run.connect(pgui.CLICK, on_button_run)
gui_cnt.add( button_run, 30, 25 )


# ---- Clear-Button ------------------------------

def on_button_clear(_widget):
    key_matrix.set_all(0)
    
button_clear = pgui.Button('Clear', width=60, height=50)
button_clear.connect(pgui.CLICK, on_button_clear)
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


### ---- Bank select-Radio buttons ------------------------------
##
##radio_bank_label = pgui.Label("Bank" , font=font_normal, color=(230,230,230))
##group_bank = pgui.Group(name='bank_select', value=0)
##table_bank = pgui.Table(width=200, height=20)
##table_bank.tr()
##for i in range(10):
##    if i%5 == 0:
##        table_bank.td( pgui.Spacer(width=10, height=20) )
##    table_bank.td( pgui.Radio(group_bank, i) )
##    
##
##gui_cnt.add( radio_bank_label, 420, 25 )
##gui_cnt.add( table_bank, 420, 55 )


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

def load_matrix(filename='sequence.dat'):
    f = open(filename)
    m = [ [int(d) for d in line.split()] for line in  f.read().splitlines() ]
    key_matrix.set_matrix( m )
    f.close()

try: load_matrix()
except: print 'Error loading sequence'

# -----------------------------------------------------------------------------
# Main loop state
# -----------------------------------------------------------------------------

main_run = 1           # Set to 0 in main loop to terminate program
main_t_next = None     # Timestamp of next main loop iteration

seq_run = 0         # Set to 1 to start sequencer, set to 0 to stop sequencer.
seq_running = 0     # Flags that the sequencer is actually running. Do NOT modify maually!

seq_step = None   # Current beat step in sequence
seq_t_next = None # Timestamp of next beat in sequence


def seq_start():
    global seq_run
    if not seq_run: seq_run = 1; button_run.value = 'Stop'

def seq_stop():
    global seq_run
    if seq_run: seq_run = 0; button_run.value = 'Play'

# -----------------------------------------------------------------------------
# Main loop 
# -----------------------------------------------------------------------------

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
                    if enable_midi and play_midi: send_midi(key.channel)
                    if play_trbold:
                        send_trbold( key.channel+1 )
                        ##print 'trbold:', key.channel+1
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
            ##print seq_step, '  +%d ms' % int(1e3*(timer()-seq_t_next))
            seq_t_next +=  60./bpm  # time of next beat

            if play_click:
                if seq_step%8==0: tick.play()
                else: tack.play()
            
            if enable_midi and play_midi:
                # For a drum set, we only send Note On events
                for key in key_matrix.get_keys(seq_step):
                    if key.active:
                        send_midi( key.channel )

            # Trommelbold
            if play_trbold:
                trbold_chans = \
                    [ n+1 for (n,key) in enumerate( key_matrix.get_keys(seq_step) ) \
                         if key.active ]
                send_trbold( trbold_chans )
                print 'trbold:', trbold_chans
            
                        
        # Screen
        screen.fill((0,0,0))
        key_matrix.draw(seq_step)
        gui.paint(screen)
        pygame.display.flip()


        # Main loop clock
        if seq_running:
            if main_t_next == None: main_t_next = timer()
            main_t_next += main_dt
            while timer() < main_t_next:
                if timer()<main_t_next-0.02:
                    pass
                    ##time.sleep(0.001)
                else: pass
        else:  # not running
            time.sleep(0.010)
                

                


finally:
    pygame.quit()
    if play_midi:
        midi_out.close()
        pygame.midi.quit()
        # XXX: Throws an error when closing. Seems to be a bug in pygames portmidi wrapper
