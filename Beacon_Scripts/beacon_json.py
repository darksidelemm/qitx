#!/usr/bin/env python
# QITX Beacon Script
#
# This needs to be run in the minute before the transmission is scheduled to start.
#
# Requires 2 files, a station config file (see example.cfg), and a schedule file.
# The schedule file is a json file, containing an array of python dictionaries.
# Each dictionary must contain at the least, a "mode" and "starttime" field.
# See the example_schedule.json file.
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

import time,sys,random,string,ConfigParser,QITX,binascii,json
from datetime import datetime,timedelta

configfile = "./example.cfg"

schedulefile = "./psk_beacon.json"


def read_config(filename=configfile):
    # Read in settings from provided config file path.
    conf = {}
    config = ConfigParser.RawConfigParser()
    try:
        config.read(filename)
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
    
    return conf

def read_schedule(filename=schedulefile):
    try:
        f = open(filename)
        fd = f.read()
        schedule = json.loads(fd)
    except:
        print "Error reading schedule"
        sys.exit(1)
        
    return schedule
    
def print_schedule(sked):
    print "\nSchedule as follows:\n"
    print "Time, Mode"
    for s in sked:
        print str(s["starttime"]) + " , " + s["mode"]

def print_config(conf):
    print "Callsign:           " + conf["mycall"]
    print "Transmit Frequency: " + conf["freq"]
    print "Transmit Sideband:  " + conf["freqssb"]
    print "Transmit Offset:    " + conf["freqoff"]
    print "Transmit Power:     " + conf["power"]

def generate_txstring():
    
    txdata = {} 
    
    #Generate our random string, using the current date and time as a seed.
    next_minute = datetime.utcnow() + timedelta(0,0,0,0,1,0)
    
    txdata["timeseed"] = next_minute.strftime("%d-%m-%Y %H:%M")
    
    new_random = random.WichmannHill()
    new_random.seed(binascii.crc32(txdata["timeseed"]))
    txdata["randomdata"] = ''.join(new_random.choice(string.ascii_uppercase + string.ascii_lowercase + string.digits) for x in range(64))
    txdata["txstring"] = "DE VK5QI VK5QI $$$$$" + txdata["randomdata"] + "$$$$$" 
    
    return txdata


def psk(tx,mode,data):

    if(mode == "BPSK31"):
        print "Transmitting: PSK31"
        tx.s.write("PSKTERM,31\n")
        interchar_delay = 0.4
    elif(mode == "BPSK63"):
        print "Transmitting: PSK63"
        tx.s.write("PSKTERM,63\n")
        interchar_delay = 0.25
    elif(mode == "BPSK125"):
        print "Transmitting: PSK125"
        tx.s.write("PSKTERM,125\n")
        interchar_delay = 0.1
    elif(mode == "BPSK250"):
        print "Transmitting: PSK250"
        tx.s.write("PSKTERM,250\n")
        interchar_delay = 0.1
    else:
        print "Invalid BPSK Mode"
        return -1
        
    print "Expected TX Time: " + str(len(data)*interchar_delay + 4) + " seconds."
    
    # Transmit phase reversals for a few seconds, to allow the receiver to lock on
    time.sleep(3)
    # Transmit our string character by character
    for x in data:
        tx.s.write(x)
        time.sleep(interchar_delay)
    
    # Transmit phase reversals for a second, then end transmission.
    time.sleep(1)
    tx.s.write("\x04")
    
    # Wait for Modem to report it has finished
    while tx.s.inWaiting()<4 :
        time.sleep(0.5)
        
    data = tx.s.readline()
    if data.startswith('OK'):
        print "... OK"
    else:
        print "... something broke?"

    return 0


# Main Script
wait_for_minute = True

if len(sys.argv)>=3:
    configfile = sys.argv[1]
    schedulefile = sys.argv[2]

if len(sys.argv)>=4:
    wait_for_minute = False
conf =  read_config(configfile)
schedule =  read_schedule(schedulefile)

print_config(conf)
print_schedule(schedule)
txdata = generate_txstring()
print "Start Time: " + txdata["timeseed"]
print "Data Sequence: " + txdata["randomdata"]


# Connect to the transmitter
tx = QITX.QITX(conf['serial_device'])

# Setup the transmitter
tx.set_parameter("FREQ",conf['freq'])
tx.set_parameter("FREQOFF",conf['freqoff'])
tx.set_parameter("FREQSSB",conf['freqssb'])
tx.set_parameter("POWER",conf['power'])
tx.set_parameter("CALL",conf['mycall'])

# Turn the PA on.
tx.power_on(True)

print "Waiting until start of minute."
# Wait until 0 seconds (start of next minute)
while(time.gmtime().tm_sec != 0 and wait_for_minute):
    time.sleep(0.1)

start_time = int(time.time())

for tx_setting in schedule:
    # Wait until the set start time
    print "Next: " + str(tx_setting["starttime"]) + " , " + tx_setting["mode"]
    
    # Set up frequency now:
    if "freq" in tx_setting:
        tx.set_parameter("FREQ",str(tx_setting['freq']))
    
    # Wait until the correct time to start. Transmit immediately if we're past that time for some reason.
    while (int(time.time()) - start_time) < int(tx_setting["starttime"]):
        time.sleep(0.1)

    if(tx_setting["mode"] == "IDENT"):
        tx.transmit_ident()
    elif(tx_setting["mode"].find("BPSK") != -1):
        psk(tx,tx_setting["mode"],txdata["txstring"])


tx.power_on(False)
tx.close()



