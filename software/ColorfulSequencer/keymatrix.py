
import pygame, colorsys

def empty_list(shape):
    if len(shape)==1: return [ None for i in range(shape[0]) ]
    else: return [empty_list(shape[1:]) for i in range(shape[0])] 


class colors:
    key_off = [(30,30,30)]*8
    ##key_on = [(150,70,70)]*8
    key_on = [ tuple( int(i*255) for i in colorsys.hsv_to_rgb(c/8., 1.0, 0.5))
               for c in range(8) ]


class Key:
    rect = None
    color = colors.key_off[0]
    active = 0
    step = None
    channel = None
    def __init__(self, step=None, channel=None):
        self.step, self.channel = step, channel
    def set_active(self, active=1):
        self.active = active
        self.color = colors.key_on[self.channel%8] if active \
                            else colors.key_off[self.channel%8]
    def toggle_active(self):
        self.set_active( not self.active )
        


class KeyMatrix:
    nsteps = None
    nchannels = None
    steps = None    # [nsteps][nchannels]
    channels = None # [nchannels][nsteps]
    keys = None  # All keys
    beat = None
    surface = None

    def __init__( self, nsteps=8, nchannels=8 ):
        self.nsteps = nsteps
        self.nchannels = nchannels

        # Create key lists:
        self.steps = empty_list((nsteps,nchannels))     # [step][channel]
        self.channels = empty_list((nchannels,nsteps))  # [channel][step] 
        self.keys = []   # flat list of all keys

        # Populate key lists
        for s in range(nsteps):
            for c in range(nchannels):
                k = Key(s, c)
                self.keys.append(k)
                self.steps[s][c] = k
                self.channels[c][s] = k


    def place( self, surface, rect ):
        """Place Keyboard matrix on given surface inside a given rect.
        This is where it will be rawn by draw()."""

        self.surface = surface
        self.rect = rect
        
        X0,Y0, W, H = rect
        margin = 5  # (pixels around colored key rect)
        nx, ny = self.nsteps, self.nchannels  # number of rows, columns
        w = int( (W-(nx+1)*margin)/nx )  # width of one key rect 
        h = int( (H-(ny+1)*margin)/ny )  # height

        # Place all key rects
        for k in self.keys:
            x0 = int( X0 + margin + (w + margin) * k.step )
            y0 = int( Y0 + margin + (h + margin) * k.channel )
            k.rect = x0, y0, w, h
            

    def draw( self, step=None ):
        """Draw keyboard matrix to surface given previously via place()."""

        if self.surface == None: return

        # Draw all key rects
        for k in self.keys:
            color = [min(2*c,255) for c in k.color] \
                      if k.step==step else k.color
            pygame.draw.rect( self.surface, color, k.rect )    


    def click( self, (x,y) ):
        """Evaluate mouse clicks. If a mouse click is inside a key's area, the key will be toggled.
        The key positions on the screen are determiend by the place function. Prior to the first
        call to place(), the click() function will have no effect"""
        for k in self.keys:
            if k.rect==None: continue
            x0,y0,w,h = k.rect
            if x >= x0 and x < x0+w and y >= y0 and y < y0+h:
                k.toggle_active()
                return k
              


    def get_keys( self, step=None, channel=None):
        """Get a list of keys of the respective step or channel or (step and channel)"""
        if step==None:
            return self.channels[channel] if channel<self.nchannels else []
        if channel==None:
            return self.steps[step] if step<self.nsteps else []
        return self.steps[step][channel] if (steps<self.nsteps and channel<nchannels) else []

    def get_matrix( self ):
        m = [[ chan.active for chan in step] for step in self.steps]
        return m

    def set_matrix( self, m ):
        for i,step in enumerate(self.steps):
            for j,key in enumerate(step):
                key.set_active( m[i,j] )

    def set_all( self, a ):
        for i,step in enumerate(self.steps):
            for j,key in enumerate(step):
                key.set_active( a )
        



km = KeyMatrix(8,8)
m = km.get_matrix()
