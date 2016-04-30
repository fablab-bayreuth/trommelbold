
import time
import serial
from serial.tools.list_ports import comports

def list_ports():
    ports = [p[0] for p in comports()]
    return ports


class TrommelboldCom(object):
    '''Serial interface class for Arduino Trommelbold'''
    _port = None
    portname = None

    def __init__(self, portname=None, baudrate=None):
        if portname!= None:
            self.open(portname, baudrate)

    def open(self, portname, baudrate=19200):
        if baudrate==None: baudrate=19200
        self.close()
        print 'Open Trommelbold on port' + portname + '...'
        self._port = serial.Serial(portname, baudrate=baudrate, timeout=1)
        # Note: By default, the Arduino resets when opening the Com port, needs
        #   1-2 seconds to boot. The firmware sends a greeting when up and running.
        self._port.timeout = 0.05
        if not self.is_trommelbold():
            # May be busy booting, wait for greeting message
            self._port.timeout = 3
            self.readline()
            self._port.flushInput()
            if not self.is_trommelbold():
                # If still not responding, something is wrong
                self._port.close()
                print "Error: Did not recognize Trommelbold on port %s" % portname
                return
        self.portname = portname
        print 'Ok'

    def is_open(self):
        if self._port != None:
            return self._port.isOpen()
        else: return False

    def close(self):
        if self._port != None:
            self._port.close()
        self._port = None
        self.portname = None

    def __del__(self):
        try: self.close()
        except: pass

    def write(self, data):
        if not self.is_open():
            print 'Error: cannot write, serial port not open.'
            return
        if not isinstance(data,str): return
        if not (data[-1] in ['\r', '\n'] ):
            data += '\r'
        self._port.write(data)

    def readline(self):
        if not self.is_open():
            print 'Error: cannot read, serial port not open.'
            return
        return self._port.readline().strip()

    def ask(self, cmd):
        self.write(cmd)
        return self.readline()

    def get_id( self ):
        self._port.flushInput()
        return self.ask('id?')

    def is_trommelbold( self ):
        self._port.flushInput()
        a = self.ask('trommelbold?')
        return a.lower().startswith('yessir!')

    def hit( self, chan ):
        '''Hit drum on channel. <chan> can also be a list of channels to hit.'''
        if isinstance(chan, int):
            msg = 'h%d' % chan
        else:
            try: msg = ''.join( ['h%d'%int(ch) for ch in chan] )
            except: print 'Error: invalid channel list:' + str(chan)
        self.write( msg )

    def release( self, chan ):
        '''Hit channel. <chan> can also be a list of channels to release.'''
        if isinstance(chan, int):
            msg = 'r%d' % chan
        else:
            try: msg = ''.join( ['r%d'%int(ch) for ch in chan] )
            except: print 'Error: invalid channel list:' + str(chan)
        self.write( msg )
    
