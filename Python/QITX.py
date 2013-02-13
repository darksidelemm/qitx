#!/usr/bin/env python
# QITX.py - QITX Communication library
#
# Copyright 2012 Mark Jessop <mark.jessop@adelaide.edu.au>
# 
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

import serial,sys,time

default_serial_device = '/dev/arduino'
default_baud_rate = 38400

class QITX(object):
    
    def __init__(self,serial_device=default_serial_device, baud_rate=default_baud_rate):
        try:
            self.s = serial.Serial(serial_device, baud_rate, timeout=1)
        except serial.serialutil.SerialException as e:
            print "ERROR:",e

    def close(self):
        self.s.close()
        
    def get_parameter(self,string):
        self.s.write(string+"\n")
        return self.get_argument()
        
    def set_parameter(self,string,arg):
        self.s.write(string+","+arg+"\n")
        if self.get_argument() == arg:
            return True
        else:
            return False
    
    def get_argument(self):
        data1 = self.s.readline()
        # Do a couple of quick checks to see if there is useful data here
        if len(data1) == 0:
            return -1
            
        # Maybe we didn't catch an OK line?
        if data1.startswith('OK'):
            data1 = self.s.readline()
            
        # Check to see if we have a comma in the string. If not, there is no argument.
        if data1.find(',') == -1:
            return -1
        
        data1 = data1.split(',')[1].rstrip('\r\n')
        
        # Check for the OK string
        data2 = self.s.readline()
        if data2.startswith('OK'):
            return data1
            
    def tx_inhibited(self):
        data = self.get_parameter('INHIBIT')
        if data == "ON":
            return True
        else:
            return False
            
    def clear_inhibit(self):
        return self.set_parameter('INHIBIT','OFF')
        
    
    def transmit_psk(self,baudrate=31):
        self.s.write('PSK,'+str(baudrate)+'\n')
        
        # Wait until we have either an OK or an ERROR in our serial RX buffer
        while self.s.inWaiting()<4 :
            time.sleep(0.5)
        
        data = self.s.readline()
        if data.startswith('OK'):
            return True
        else:
            return False
            
            
    def transmit_rtty(self,baudrate=50,shift=170):
        self.s.write('RTTY,'+str(baudrate)+','+str(shift)+'\n')
        
        # Wait until we have either an OK or an ERROR in our serial RX buffer
        while self.s.inWaiting()<4 :
            time.sleep(0.5)
        
        data = self.s.readline()
        if data.startswith('OK'):
            return True
        else:
            return False
            
    def transmit_selcall(self,source=1881,dest=1882,chantest=True):
        if chantest:
            self.s.write('SELTEST,'+str(source)+','+str(dest)+'\n')
        else:
            self.s.write('SELCALL,'+str(source)+','+str(dest)+'\n')
        
        # Wait until we have either an OK or an ERROR in our serial RX buffer
        while self.s.inWaiting()<4 :
            time.sleep(0.5)
        
        data = self.s.readline()
        if data.startswith('OK'):
            return True
        else:
            return False
    
    def transmit_ident(self):
        self.s.write('IDENT\n')
        
        # Wait until we have either an OK or an ERROR in our serial RX buffer
        while self.s.inWaiting()<4 :
            time.sleep(0.5)
        
        data = self.s.readline()
        if data.startswith('OK'):
            return True
        else:
            return False
        