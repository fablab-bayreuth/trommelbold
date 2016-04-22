#Import Modules
import pygame
from pygame.locals import *
import time
import math

# import gui stuff
from pgu import gui as pgui

screenSize = (642, 429)
lines = []
lineLimit = 20


def logSliderAction(txt):
    """ add the slider status to the 'edit' window (callback function)"""
    text = 'Slider is at ' + str(txt.value)
    lines.append(text)
    while len(lines) > lineLimit: lines[0:1] = []
    sll.set_text(str(txt.value))


#Initialize Everything
pygame.init()
pygame.font.init()
font = pygame.font.SysFont("default", 18)

screen = pygame.display.set_mode(screenSize)
pygame.display.set_caption('GUI Test - PGU')

# create GUI object
gui = pgui.App()

# layout using document
lo = pgui.Container(width=350)

# create slider label
sll = pgui.Label("Slider", font=font)
lo.add(sll,36,195)
# create slider
sl = pgui.HSlider(value=1,min=0,max=100,size=32,width=200,height=16)
sl.connect(pgui.CHANGE, logSliderAction, sl)
lo.add(sl,53,210) #, colspan=3)


gui.init(lo)

#Main Loop
run = 1
while 1:

    #Handle Input Events
    for event in pygame.event.get():
        if event.type == QUIT:
            run = 0
        elif event.type == KEYDOWN and event.key == K_ESCAPE:
            run = 0
    
        # pass event to gui
        gui.event(event)
        
    if not run: break

    # clear background, and draw clock-spinner
    screen.fill((250, 250, 250))

    # Draw GUI
    gui.paint(screen)

    pygame.display.flip()
