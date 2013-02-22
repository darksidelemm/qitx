#!/usr/bin/env python
# QITX PSK Transmitter routine
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
import time,sys,random,string,ConfigParser,QITX

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
    conf['gracetime'] = int(config.get('SiteInfo','gracetime'))
    conf['gracecount'] = int(config.get('SiteInfo','gracecount'))
except:
    print "Error reading config file"
    sys.exit(1)


# Connect to the transmitter
tx = QITX.QITX(conf['serial_device'])

# Setup the transmitter
tx.set_parameter("FREQ",conf['freq'])
tx.set_parameter("FREQOFF",conf['freqoff'])
tx.set_parameter("FREQSSB",conf['freqssb'])
tx.set_parameter("POWER",conf['power'])

print "Press Enter to stop"
# Switch into PSK Terminal mode, and transmit our string.
tx.set_parameter("CARRIER","ON")
tx.power_on(True)

temp = raw_input()

tx.power_on(False)
tx.set_parameter("CARRIER","OFF")

# Close our connection to the transmitter
tx.close()

