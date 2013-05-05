#!/usr/bin/env python
#
#    FLDIGI Beacon receiver - Data Analysis
#
#   Needs python-Levenshtein from https://github.com/miohtama/python-Levenshtein
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

import socket,sys,time,random,string,binascii,json,Levenshtein
from datetime import datetime,timedelta

class BeaconAnalysis:

    threshold = 0.5

    def __init__(self, jsonfile):
        f = open(jsonfile)
        fd = f.read()
        self.data = json.loads(fd)
        f.close()
        
    def prettyprint(self):
        print "Timestamp:     " + self.data["timeseed"]
        print "Expected Data: " + self.data["expected_data"]
        print "PSK31 Data:    " + self.data["psk_data"]
        print "PSK31 Jaro Dist: " + str(Levenshtein.jaro(self.data["expected_data"],self.data["psk_data"]))
        print "DOMEX8 Data:   " + self.data["domex_data"]
        print "DOMEX Jaro Dist: " + str(Levenshtein.jaro(self.data["expected_data"],self.data["domex_data"]))
        
    def get_max_quality(self, quality_set):
        quality_data = []
        for x in range(0,len(quality_set)):
            quality_data.append(quality_set[x][1])
        
        return max(quality_data)
        
    def get_psk_jaro(self):
        return Levenshtein.jaro(self.data["expected_data"],self.data["psk_data"])
    
    def get_domex_jaro(self):
        return Levenshtein.jaro(self.data["expected_data"],self.data["domex_data"])
    
    def get_timestamp(self):
        return self.data["timeseed"]
    
    def psk_valid(self):
        return Levenshtein.jaro(self.data["expected_data"],self.data["psk_data"]) > self.threshold
        
    def domex_valid(self):
        return Levenshtein.jaro(self.data["expected_data"],self.data["domex_data"]) > self.threshold
    
    def plot_psk(self):
        from pylab import *
        
        quality_set = self.data["psk_qual"]
        
        time_data = []
        quality_data = []
        for x in range(0,len(quality_set)):
            quality_data.append(quality_set[x][1])
            time_data.append(quality_set[x][0] - quality_set[0][0])
        plot(time_data, quality_data)
        xlabel("Time (s)")
        ylabel("Signal Quality (0-100)")
        title("PSK Signal Quality for " + self.data["timeseed"])
        show()
        
    def plot_domex(self):
        from pylab import *
        
        quality_set = self.data["domex_qual"]
        
        time_data = []
        quality_data = []
        for x in range(0,len(quality_set)):
            quality_data.append(quality_set[x][1])
            time_data.append(quality_set[x][0] - quality_set[0][0])
        plot(time_data, quality_data)
        xlabel("Time (s)")
        ylabel("Signal Quality (0-100)")
        title("DOMEX8 Signal Quality for " + self.data["timeseed"])
        show()
        
        

def main():
    if len(sys.argv) >= 2:
        a = BeaconAnalysis(sys.argv[1])
        a.prettyprint()
        if(a.psk_valid()):
            a.plot_psk()
        else:
            print "No Valid PSK data. Not Plotting."
        
        if(a.domex_valid()):
            a.plot_domex()
        else:
            print "No Valid DOMEX8 data. Not Plotting."
            
    else:
        print "Needs Argument"

if __name__ == "__main__":
    main()
