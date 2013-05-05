#!/usr/bin/env python
# QITX Beacon Script
#
# This needs to be run in the minute before the transmission is scheduled to start.
#
# Schedule is as follows:
# 0: Morse Ident
# 15: Carrier for 10 seconds.
# 30: PSK31 pseudorandom sequence
# 90: DominoEX8 pseudorandom sequence
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
import time,sys,random,string,ConfigParser,QITX,binascii
from datetime import datetime,timedelta

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
    

#Generate our random string, using the current date and time as a seed.
next_minute = datetime.utcnow() + timedelta(0,0,0,0,1,0)
timeseed = next_minute.strftime("%d-%m-%Y %H:%M")
print "Seed: " + timeseed

new_random = random.WichmannHill()
new_random.seed(binascii.crc32(timeseed))
txstring = ''.join(new_random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for x in range(32))
txstring = "DE VK5QI $$$$$" + txstring     

print "TX Sequence: " + txstring

# Connect to the transmitter
tx = QITX.QITX(conf['serial_device'])

# Are we inhibited from transmitting? Increment the grace counter
if (tx.tx_inhibited() and conf['inhibitactive']):
    conf['gracecount'] = conf['gracecount'] + 1;
    
    # If we have exceeded the 'grace' period, then reset the counter
    if(conf['gracecount'] > conf['gracetime']):
        tx.clear_inhibit()
        conf['gracecount'] = 0
        config.set('SiteInfo','gracecount',str(conf['gracecount']))
        with open(configfile,'wb') as cfgwriteout:
            config.write(cfgwriteout)
    else:
    # Else writeout the new gracecount value, and exit
        config.set('SiteInfo','gracecount',str(conf['gracecount']))
        with open(configfile,'wb') as cfgwriteout:
            config.write(cfgwriteout)
        tx.close()
        print "Transmitter Inhibited, not transmitting."
        sys.exit(0)

# Setup the transmitter
tx.set_parameter("FREQ",conf['freq'])
tx.set_parameter("FREQOFF",conf['freqoff'])
tx.set_parameter("FREQSSB",conf['freqssb'])
tx.set_parameter("POWER",conf['power'])
tx.set_parameter("CALL",conf['mycall'])
tx.set_parameter("MSG",txstring)

# Flush input
while(tx.s.inWaiting()>0):
    print(tx.s.read())

print "Ready to Transmit... waiting"

# Ident
tx.power_on(True)

# Wait until 0 seconds (start of next minute)
while(time.gmtime().tm_sec != 0):
    pass

print "Transmitting: Ident"
tx.transmit_ident()  # Approx 15 seconds

# 10 seconds of carrier
print "Transmitting: Carrier"
tx.set_parameter("CARRIER","ON")
time.sleep(10);
tx.set_parameter("CARRIER","OFF")

# Wait until 30 seconds 
while(time.gmtime().tm_sec != 30):
    pass
    
# Switch into PSK Terminal mode, and transmit our string.
# This may take anywhere up to 1 minute to transmit.
print "Transmitting: PSK"
tx.s.write("PSKTERM,31\n")
time.sleep(3)
# Transmit our string character by character
for x in txstring:
        tx.s.write(x)
        time.sleep(0.4)
        
time.sleep(3)

tx.s.write("\x04")
while tx.s.inWaiting()<4 :
    time.sleep(0.5)
        
data = tx.s.readline()
if data.startswith('OK'):
    print "... OK"
else:
    print "... something broke?"


# Wait until 30 seconds, this will be in the next minute.
print "Waiting for +1:30"
while(time.gmtime().tm_sec != 30):
    pass

print "Transmitting: DominoEX8"
tx.transmit_domino()

tx.power_on(False)
print "Done."

# Close our connection to the transmitter
tx.close()