#!/usr/bin/env python
#
#    FLDIGI Beacon receiver - Data Analysis
#
#    Parses a folder of json files using the BeaconAnalysis code
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

import os,glob,Levenshtein,json,sys
import BeaconAnalysis

if len(sys.argv) < 2:
    print "Need folder as argument"
    sys.exit(1)
else:
    pass

filemask = "./" + sys.argv[1] + "/*.json"

filenames = sorted(glob.glob(filemask))
data = []

print "time,psk_jaro,domex_jaro"

for filename in filenames:
    b = BeaconAnalysis.BeaconAnalysis(filename)
    
    print b.get_timestamp() + "," + str(b.get_psk_jaro()) + "," + str(b.get_domex_jaro())