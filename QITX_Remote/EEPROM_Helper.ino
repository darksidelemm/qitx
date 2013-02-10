/*
	QITX
	EEPROM Helper Code
	
	Copyright (C) 2012 Mark Jessop <mark.jessop@adelaide.edu.au>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a full copy of the GNU General Public License, 
    see <http://www.gnu.org/licenses/>.
*/
#include <ctype.h>
#include <EEPROM.h>

#define CALLSIGN_LOCATION	0
#define	CALLSIGN_LENGTH		6

#define FREQ_LOCATION		6
#define FREQ_LENGTH			4

#define FREQ_OFF_LOCATION		10
#define FREQ_OFF_LENGTH			4

#define FLAGS_LOCATION		14


void store_callsign(char * call){
	for(int i = CALLSIGN_LOCATION; i<CALLSIGN_LENGTH; i++){
		if(isalnum(call[i])){
			EEPROM.write(i,call[i]);
		}else{
			EEPROM.write(i,' ');
		}
	}
}

void read_callsign(char * call_store){
	for(int i = CALLSIGN_LOCATION; i<CALLSIGN_LENGTH; i++){
		char c = EEPROM.read(i);
		if(isalnum(c)){
			call_store[i] = c;
		}else{
			call_store[i] = ' ';
		}
	}
	call_store[6] = 0;
}

void store_frequency(unsigned long freq){
	byte Byte1 = ((freq >> 0) & 0xFF);
	byte Byte2 = ((freq >> 8) & 0xFF);
	byte Byte3 = ((freq >> 16) & 0xFF);
	byte Byte4 = ((freq >> 24) & 0xFF);

  EEPROM.write(FREQ_LOCATION, Byte1);
  EEPROM.write(FREQ_LOCATION + 1, Byte2);
  EEPROM.write(FREQ_LOCATION + 2, Byte3);
  EEPROM.write(FREQ_LOCATION + 3, Byte4);
}

unsigned long read_frequency(){
	byte Byte1 = EEPROM.read(FREQ_LOCATION);
  byte Byte2 = EEPROM.read(FREQ_LOCATION + 1);
  byte Byte3 = EEPROM.read(FREQ_LOCATION + 2);
  byte Byte4 = EEPROM.read(FREQ_LOCATION + 3);

  long firstTwoBytes = ((Byte1 << 0) & 0xFF) + ((Byte2 << 8) & 0xFF00);
  long secondTwoBytes = (((Byte3 << 0) & 0xFF) + ((Byte4 << 8) & 0xFF00));
  secondTwoBytes *= 65536; // multiply by 2 to power 16 - bit shift 24 to the left

  return (firstTwoBytes + secondTwoBytes);
}

void store_frequency_offset(unsigned long freq){
	byte Byte1 = ((freq >> 0) & 0xFF);
	byte Byte2 = ((freq >> 8) & 0xFF);
	byte Byte3 = ((freq >> 16) & 0xFF);
	byte Byte4 = ((freq >> 24) & 0xFF);

  EEPROM.write(FREQ_OFF_LOCATION, Byte1);
  EEPROM.write(FREQ_OFF_LOCATION + 1, Byte2);
  EEPROM.write(FREQ_OFF_LOCATION + 2, Byte3);
  EEPROM.write(FREQ_OFF_LOCATION + 3, Byte4);
}

unsigned long read_frequency_offset(){
	byte Byte1 = EEPROM.read(FREQ_OFF_LOCATION);
  byte Byte2 = EEPROM.read(FREQ_OFF_LOCATION + 1);
  byte Byte3 = EEPROM.read(FREQ_OFF_LOCATION + 2);
  byte Byte4 = EEPROM.read(FREQ_OFF_LOCATION + 3);

  long firstTwoBytes = ((Byte1 << 0) & 0xFF) + ((Byte2 << 8) & 0xFF00);
  long secondTwoBytes = (((Byte3 << 0) & 0xFF) + ((Byte4 << 8) & 0xFF00));
  secondTwoBytes *= 65536; // multiply by 2 to power 16 - bit shift 24 to the left

  return (firstTwoBytes + secondTwoBytes);
}