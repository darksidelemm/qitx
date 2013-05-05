#!/usr/bin/env python
#
#    FLDIGI Beacon receiver
#
#   This program assumes it has been started on the minute the beacon starts transmitting.
#   Is listens for PSK31 at +30 seconds, then DominoEX8 at +90 seconds.
#    
#    Copyright (C) 2013 Mark Jessop <mark.jessop@adelaide.edu.au>
# 
#      This program is free software: you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation, either version 3 of the License, or
#      (at your option) any later version.
# 
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
#      GNU General Public License for more details.
# 
#      For a full copy of the GNU General Public License, 
#      see <http://www.gnu.org/licenses/>.
#
#

from xmlrpclib import ServerProxy, Error
import socket,sys,time,random,string,binascii,json
from datetime import datetime,timedelta

# Assume fldigi is running on the local machine.
xmlrpc_addr = "http://localhost:7362"
flarq_host = "localhost"
flarq_port = 7322

# Centre frequencies for the various modem tones. These may vary with transmitter temperature,
# but should be a good enough start point for fldigi to find it.
psk_center = 1007
domino_center = 1140
squelch_threshold = 2.0

# Can set this to False for debugging purposes.
delay = True

#Generate our expected random string
next_minute = datetime.utcnow()
timeseed = next_minute.strftime("%d-%m-%Y %H:%M")
print(timeseed)

new_random = random.WichmannHill()
new_random.seed(binascii.crc32(timeseed))
txstring = ''.join(new_random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for x in range(32))
txstring = "DE VK5QI $$$$$" + txstring  
print "Expecting: " + txstring

# Open a connection to the fldigi XMLRPC server (or die trying)
fldigi = ServerProxy(xmlrpc_addr)

# Set Squelch level very high, so we limit the data getting through
fldigi.main.set_squelch_level(90.0)

# Open a connetion to the FLARQ socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(1.0)
s.connect((flarq_host,flarq_port))


# PSK Setup
fldigi.modem.set_by_name("BPSK31")

print "Setup, waiting for +30"

# Wait until +30 seconds 
if delay:
    while(time.gmtime().tm_sec != 30):
        time.sleep(0.1)

# Set Squelch level to the normal level
fldigi.main.set_squelch_level(squelch_threshold)
# Finish modem setup, this should lock onto the PSK signal within the 3 second preamble.
fldigi.modem.set_carrier(psk_center)

lock = False
pskdata = ""

psk_qual = []

print "Attempting to receive BPSK31..."
# Try and receive until +60 seconds
while(time.gmtime().tm_sec != 0):
    time.sleep(0.1)
    current_quality = fldigi.modem.get_quality()
    psk_qual.append([time.time(), current_quality])
    if(current_quality>squelch_threshold or lock):
        lock = True
        temp = ""
        try:
            temp = s.recv(8)
        except:
            pass
#        sys.stdout.write(temp)
#        sys.stdout.flush()
        pskdata = pskdata + temp

print ""

if(len(pskdata) == 0):
    print "No PSK data received."
else:
    print "Got: " + pskdata

pskdata = ''.join(filter(lambda x:x in string.printable, pskdata))

# Attempt to receive DominoEX8
fldigi.modem.set_by_name("DOMEX8")

# Set Squelch level very high, so we limit the data getting through
fldigi.main.set_squelch_level(90.0)

# Wait for +90 (which will be 30 seconds into the minute)
if delay:
    while(time.gmtime().tm_sec != 28):
        time.sleep(0.1)

# Set Squelch level to the normal level
fldigi.main.set_squelch_level(squelch_threshold)
# Finish modem setup, this should lock onto the DOMEX8 signal within the 3 second preamble.
fldigi.modem.set_carrier(domino_center)

domex_data = ""
domex_qual = []

print "Attempting to receive DOMEX8..."
# Try and receive until +60 seconds
while(time.gmtime().tm_sec != 0):
    time.sleep(0.1)
    current_quality = fldigi.modem.get_quality()
    domex_qual.append([time.time(), current_quality])
    if(current_quality>squelch_threshold):
        temp = ""
        try:
            temp = s.recv(8)
        except:
            pass
#        sys.stdout.write(temp)
#        sys.stdout.flush()
        domex_data = domex_data + temp

print ""

domex_data = ''.join(filter(lambda x:x in string.printable, domex_data))

if(len(domex_data) == 0):
    print "No DOMEX8 data received."
else:
    print "Got: " + domex_data
    
s.close()


# Write received data out to a file.
dirname = "./" + timeseed[:-6]

if not os.path.exists(dirname):
    os.makedirs(dirname)

filename = dirname + "/" + timeseed + ".json"

output = {}
output["timeseed"] = timeseed
output["expected_data"] = txstring
output["psk_data"] = pskdata
output["domex_data"] = domex_data
output["psk_qual"] = psk_qual
output["domex_qual"] = domex_qual

target = open(filename,'w')
target.write(json.dumps(output))
target.close()



