#!/usr/bin/env python
# QITX Hard Reset routine
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
import time,sys,random,string,ConfigParser,QITX,serial

configfile = "./example.cfg"

# Read in settings from provided config file path.
config = ConfigParser.RawConfigParser()
try:
    config.read(configfile)
    conf = {}
    conf['serial_device'] = config.get('DeviceInfo','serial_device')
    conf['power_pin'] = config.get('DeviceInfo','power_control_pin')
    conf['mycall'] = config.get('SiteInfo','mycall')
    conf['sitename'] = config.get('SiteInfo','sitename')
    conf['freq'] = config.get('SiteInfo','freq')
    conf['freqoff'] = config.get('SiteInfo','freqoff')
    conf['freqssb'] = config.get('SiteInfo','freqssb')
    conf['power'] = config.get('SiteInfo','power')
    conf['inhibitactive'] = config.getboolean('SiteInfo','inhibitactive')
    conf['gracetime'] = int(config.get('SiteInfo','gracetime'))
    conf['gracecount'] = int(config.get('SiteInfo','gracecount'))
except:
    print "Error reading config file"
    sys.exit(1)
    
power_pin = int(conf['power_pin'])    
    
import RPi.GPIO as GPIO 
GPIO.setmode(GPIO.BCM)
GPIO.setup(power_pin,GPIO.OUT)
GPIO.output(power_pin,False)
    
ser = serial.Serial(
	port=conf['serial_device'],
	baudrate=1200,
	parity=serial.PARITY_NONE,
	stopbits=serial.STOPBITS_ONE,
	bytesize=serial.EIGHTBITS
)
ser.isOpen()
ser.close()
