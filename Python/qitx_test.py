#!/usr/bin/env python
# Quick test script to check QITX.py functionality

import QITX

tx = QITX.QITX('/dev/tty.usbmodemfd121')

print "Current Frequency: ",tx.get_parameter('FREQ')
print "Sideband: ",tx.get_parameter('FREQSSB')
print "BFO Freq: ",tx.get_parameter('FREQOFF')
print "Power: ",tx.get_parameter('POWER')
print "TX Message: ",tx.get_parameter('MSG')
print "Callsign: ",tx.get_parameter('CALL')

tx.close()
