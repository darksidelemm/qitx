/*
	QITX
	Remote Transmitter Software
	
	Code to control an AD9834 based HF Data transmitter
	
	Copyright (C) 2013 Mark Jessop <mark.jessop@adelaide.edu.au>

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
    
    
    Command Reference:
    ------------------
    All commands consist of an identifier, followed by one or more parameters delinieated by commas, then a newline.
    i.e.:
    COMMAND,<param1>,<param2><LF>
    
    A command will return OK<LF> if successful, and ERROR<LF> if not. Some commands (particuarly transmit commands)
    may take a significant amount of time to return.
    
    Sending a command which takes parameters, without sending the parameters, will return the set parameters.
    
    Frequency Settings:
    
    FREQ,<freq> 		- Set the BFO Frequency of the transmitter, in Hz.
    FREQSSB,<LSB/USB>	- Set the radio to either USB or LSB mode.
    FREQOFF,<freq> 		- Set the offset of the data centre frequency from the BFO frequency.
    
    Example: FREQ=7040000, FREQSSB=LSB, FREQOFF=1000 would result in a carrier at 7039KHz. A RTTY transmission with 170Hz 
    shift would have the mark frequency at 7039KHz and space at 7038.830KHz.
    
    Power Control:
    
    POWER,<HIGH/LOW>	- Switch between 5V and 12V supplies, giving ~800mW or 5W TX power respectively.
    					  Note: This will only work if a relay to switch power is hooked up to pin <UNKNOWN>.
    INHIBIT,<ON/OFF>    - Reads or sets the state of the INHIBIT variable.
                        - This variable can be set by pressing a button wired between pin 0 and GND.
    					
    Message Control:
    
    MSG,<string>		- Set the string to be transmitted. Max 64 characters.
    CALL,<CALLSIGN>		- Set the user callsign (max 6 characters). This is used in the IDENT command.
    
    Modulation:
    All transmissions will time out after 10 minutes.
    
    IDENT				- Transmit a 20WPM Morse ident: DE <CALLSIGN> TEST BEACON.
    CARRIER,<ON/OFF>	- Transmit a carrier.
    PSK					- Send using BPSK31. Will use phase shaping code.
    PSK,<31/63/125/250>	- Send using Phase Shift Keying, with the specified symbol rate.
    RTTY,<baudrate>,<shift> - Send using RTTY, with a specified symbol rate, 8 bits/char, and shift (Hz)
    
    SELCALL,<source>,<dest> - Send a CCIR 493-4 call request, from <source> to <dest> ID. Both IDs must be 4 digit
    						  numeric IDs.
    						  
    						  
    
*/
#include <stdlib.c>
#include <string.h>
#include <errno.h>
#include <SPI.h>
#include <AD9834.h>
#include <TimerOne.h>

//#define DEBUG_ON	1

// Pin definitions
#define LED_PIN	            13
#define BUTTON_PIN          0
#define BUTTON_INTERRUPT    2
#define POWER_CONTROL_PIN	5
#define POWER_MASTER_PIN	4

// Some defines for code readability
#define POWER_HIGH	1
#define POWER_LOW	0
#define SSB_USB		1
#define SSB_LSB		0
#define FREQ_LIMIT_LOWER	0 //7000000
#define FREQ_LIMIT_UPPER	14350000
#define MSG_LEN_LIMIT	128

// Settings.
unsigned long tx_freq = 7045000; // Transmit Frequency, Hz. This gets overwritten when reading from the EEPROM.
unsigned long tx_offset = 1000;
int tx_sideband = SSB_LSB;
int tx_power = POWER_LOW;
// Arrays for callsign and transmit message storage.
char callsign[7] = "VK5QI";
char txmessage[MSG_LEN_LIMIT] = "TEST BEACON";




// Input buffer for reading user input.
#define INPUTBUFLEN	64
//char inputBuffer[INPUTBUFLEN];
int inputBufferPtr = 0;
char tempBuffer[20]; // Buffer for string to int conversion.

String inputBuffer = "";
boolean stringComplete = false;

#define TX_BUFFER_SIZE  64

struct ring_buffer
{
  unsigned char buffer[TX_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
};

ring_buffer data_tx_buffer = { { 0 }, 0, 0};



#define TX_TIMEOUT	600000 // Timeout in milliseconds (set to 10 minutes)
int rf_on = 0;
unsigned long tx_timer = 0;

// Variable for the tx inhibit button
volatile uint8_t button_state = 0;


void setup(){
	Serial.begin(115200); // Start the USB-Serial interface
	while(!Serial);	// Wait here until a connection is made.
	
	// Setup some IO lines for output.
	pinMode(LED_PIN, OUTPUT);
	pinMode(POWER_CONTROL_PIN, OUTPUT);
	setPower();
	
	// Note: You need to activate the interrupt BEFORE activating the internal pullups.
	// This means the interrupt will likely activate at least once on boot.
	attachInterrupt(BUTTON_INTERRUPT,button_isr,FALLING);
    pinMode(BUTTON_PIN,INPUT_PULLUP);
    // Reset the button state variable, just in case the interrupt has fired 
    button_state = 0;
    digitalWrite(LED_PIN, LOW);

	pinMode(5, OUTPUT);
	digitalWrite(5, LOW);
	
	// Read in the data stored in the EEPROM.
	read_callsign(callsign);
	tx_freq = read_frequency();
	tx_offset = read_frequency_offset();
	
	// Generate the TX message, including the user callsign.
	sprintf(txmessage,"DE %s TEST BEACON",callsign);
	
	// Initialise the DDS
	AD9834_Setup(11,3,2);
	programFreq();
  	AD9834_SelectPREG(0);
  	AD9834_Sleep(0);
	AD9834_Reset(0);
	AD9834_DAC_ON(0);

}

void loop(){

	while(1){
		if(Serial.available()){
			// Read in a byte.
			char inChar = (char)Serial.read();
		
			// Work out what the character is. If letters or numbers, add to
			// the buffer, if not, don't add.
			
			// If character is a newline, we have reached the end of the string.
			if( (inChar == '\n') || (inputBufferPtr>INPUTBUFLEN)){
				// Parse string
				if( parseCommand(inputBuffer)==0 ){
					Serial.println("OK");
				}else{
					Serial.println("ERROR");
				}
				inputBuffer = "";
				inputBufferPtr = 0;
			}else if( isalnum(inChar) || ispunct(inChar) || isblank(inChar) ){
				// Add to the input buffer if printable and not a line ending.
				inputBuffer += inChar;
				inputBufferPtr++;
			}
			
		}
		// Check we haven't gone over our TX timeout limit.
		// TODO: Move this to an interrupt based system.
		if(rf_on){
			if( (millis()-tx_timer) > TX_TIMEOUT){
				tx_off();
				Serial.println("TIMEOUT");
				break;
			}
		}
				
	}
}

int parseCommand(String input){
	int params = 0;
	
	if(input.length()<3){
		// No command is shorter than 3 characters. Error.
		return 1;
	}
	// At this point we have found some sort of command.
	
	// Search through for commas, populate an array of pointers, make pointer = -1 if nothing found.
	int separators[] = {-1,-1,-1,-1,-1,-1};

	separators[0] = input.indexOf(",");
	
	if(separators[0] != -1){ // If we find one comma, maybe there are more...
		int i = 1;
		while((separators[i-1] != -1) && i<6){
			params++;
			separators[i] = input.indexOf(",", separators[i-1]+1);
			i++;
		}
	}
#ifdef DEBUG_ON
	Serial.print("Command: "); Serial.println(input.substring(0,separators[0]));
	Serial.print("Num Params: "); Serial.println(params);
#endif
	// Now we have the following variables:
	// params: the number of parameters found.
	// separators: an array containing the pointers to the commas delimiting each command.
	
	
	// To extract parameters, we iterate over the list, extracting substrings.
//	for(int i = 0; i<params; i++){
//		if(separators[i+1]!=-1){
//			Serial.println(input.substring(separators[i]+1, separators[i+1]));
//		}else{
//			Serial.println(input.substring(separators[i]+1));
//			break;
//		}
//	}
	
	
	if(params==0){ // Either a request command or a command with no parameter
		if(input.startsWith("FREQSSB")){
			Serial.print("FREQSSB,");
			if(tx_sideband) Serial.println("USB");
			else Serial.println("LSB");
			return 0;
		}else if(input.startsWith("FREQOFF")){
			Serial.print("FREQOFF,"); Serial.println(tx_offset);
			return 0;
		}else if(input.startsWith("FREQ")){
			Serial.print("FREQ,"); Serial.println(tx_freq);
			return 0;
		}else if(input.startsWith("POWER")){
			Serial.print("POWER,");
			if(tx_power) Serial.println("HIGH");
			else Serial.println("LOW");
			return 0;
		}else if(input.startsWith("MSG")){
			Serial.print("MSG,"); Serial.println(txmessage);
			return 0;
		}else if(input.startsWith("CALL")){
			Serial.print("CALL,"); Serial.println(callsign);
			return 0;
		}else if(input.startsWith("IDENT")){
			ident();
			return 0;
		}else if(input.startsWith("PSK")){
			transmit_psk();
			return 0;
		}else if(input.startsWith("RTTY")){
			transmit_rtty();
			return 0;
		}else if(input.startsWith("INHIBIT")){
		    read_inhibit();
		    return 0;
		}else{
			// No other commands are valid without parameters. Error
			return 1;
		}
	}else if(params==1){ // Commands with one argument.
		String param1 = input.substring(separators[0]+1); // Extract the argument for passing to the function.
		
#ifdef DEBUG_ON
		Serial.println(param1);
#endif
		if(input.startsWith("FREQSSB")){
			return parseSSB(param1);
		}else if(input.startsWith("FREQOFF")){
			return parseFreqOffset(param1);
		}else if(input.startsWith("FREQ")){
			return parseFreq(param1);
		}else if(input.startsWith("POWER")){
			return parsePower(param1);
		}else if(input.startsWith("MSG")){
			return parseMessage(param1);
		}else if(input.startsWith("CALL")){
			return parseCallsign(param1);
		}else if(input.startsWith("CARRIER")){
			return parseCarrier(param1);
		}else if(input.startsWith("PSKTERM")){
			return pskTerminal(param1);
		}else if(input.startsWith("PSK")){
			return parsePSK(param1);
		}else if(input.startsWith("INHIBIT")){
		    return write_inhibit(param1);
		}else{
			// No other commands are valid with only one parameter. Error
			return 1;
		}
	}else if(params==2){ // Commands with 2 parameters (RTTY, SELCALL);
		String param1 = input.substring(separators[0]+1, separators[1]);
		String param2 = input.substring(separators[1]+1);
		
#ifdef DEBUG_ON
		Serial.println(param1);
		Serial.println(param2);
#endif
		
		if(input.startsWith("RTTY")){
			return parseRTTY(param1,param2);
		}else if(input.startsWith("SELCALL")){
			return parseSelcall(param1,param2);
		}else if(input.startsWith("SELTEST")){
			return parseSeltest(param1,param2);
		}
		
	}

	return 1;
}

// Wrapper functions for enabling and disabling the DDS output.
void tx_on(){
	rf_on = 1;
	tx_timer = millis();
  	AD9834_DAC_ON(1);
	AD9834_Sign_Bit_On(1);
	digitalWrite(LED_PIN, HIGH);
}
void tx_off(){
	AD9834_DAC_ON(0);
	AD9834_Sign_Bit_On(0);
	rf_on=0;
	digitalWrite(LED_PIN, LOW);
}

// These two functions are used if you want to turn the DDS output on/off without resetting the timeout timer.
// They are mainly used by the morse code libs.
void sig_on(){
	AD9834_DAC_ON(1);
	AD9834_Sign_Bit_On(1);
	digitalWrite(LED_PIN, HIGH);
}
void sig_off(){
	AD9834_DAC_ON(0);
	AD9834_Sign_Bit_On(0);
	digitalWrite(LED_PIN, LOW);
}

void setPower(){
	if(tx_power) digitalWrite(POWER_CONTROL_PIN,HIGH);
	else	digitalWrite(POWER_CONTROL_PIN, LOW);
}

void programFreq(){
	unsigned long combinedresult = 0;
	// Calculate the actual output frequency, for verification.
	if(tx_sideband) combinedresult = tx_freq + tx_offset;
	else combinedresult = tx_freq - tx_offset;
	
	AD9834_SetFreq(1,combinedresult);
	AD9834_SetFreq(0,combinedresult);
}


int setCallsign(char* input){
	store_callsign(input+2);
	read_callsign(callsign);
	Serial.print(F("Callsign: "));
	Serial.println(callsign);
	return 0;
}

// FREQ Command
int parseFreq(String input){
	unsigned long result = 0;
	unsigned long combinedresult = 0;
	char* endptr;
	
	input.toCharArray(tempBuffer,10);
	
	result = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	
	// Calculate the actual output frequency, for verification.
	if(tx_sideband) combinedresult = result + tx_offset;
	else combinedresult = result - tx_offset;
	
	if(freqValid(combinedresult)){
		tx_freq = result;
		store_frequency(tx_freq);
		// Program the registers now, in case we're in constant carrier mode.
		programFreq();
		Serial.print("FREQ,");Serial.println(result);
		return 0;
	}else{
		//Serial.println("Invalid input.");
		return -1;
	}
}
int freqValid(unsigned long freq){
	return (freq>FREQ_LIMIT_LOWER) && (freq<FREQ_LIMIT_UPPER);
}

// FREQSSB Command
int parseSSB(String input){
	if(input.startsWith("USB")){
		tx_sideband = SSB_USB;
	}else if(input.startsWith("LSB")){
		tx_sideband = SSB_LSB;
	}else{
		return -1;
	}
	
	Serial.print("FREQSSB,");
	if(tx_sideband) Serial.println("USB");
	else Serial.println("LSB");
	return 0;
}

// POWER Command
int parsePower(String input){
	if(input.startsWith("HIGH")){
		tx_power = POWER_HIGH;
	}else if(input.startsWith("LOW")){
		tx_power = POWER_LOW;
	}else{
		return -1;
	}
	setPower();
	Serial.print("POWER,");
	if(tx_power) Serial.println("HIGH");
	else Serial.println("LOW");
	return 0;
}

// FREQOFF Command
int parseFreqOffset(String input){
	unsigned long result = 0;
	unsigned long combinedresult = 0;
	char* endptr;
	
	input.toCharArray(tempBuffer,10);
	
	result = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	
	// Calculate the actual output frequency, for verification.
	if(tx_sideband) combinedresult = tx_freq + result;
	else combinedresult = tx_freq - result;
	
	if(freqValid(combinedresult)){
		tx_offset = result;
		store_frequency_offset(tx_offset);
		// Program the registers now, in case we're in constant carrier mode.
		AD9834_SetFreq(1,combinedresult);
		AD9834_SetFreq(0,combinedresult);
		Serial.print("FREQOFF,");Serial.println(result);
		return 0;
	}else{
		//Serial.println("Invalid input.");
		return -1;
	}
}

// CARRIER Command
int parseCarrier(String input){
	if(input.startsWith("ON")){
		tx_on();
		Serial.println("CARRIER,ON");
	}else if(input.startsWith("OFF")){
		tx_off();
		Serial.println("CARRIER,OFF");
	}else{
		return -1;
	}
	return 0;
}

// MSG Command
// Doesn't do any checks on the message contents at the moment.
int parseMessage(String input){
	input.toCharArray(txmessage,MSG_LEN_LIMIT);
	
	Serial.print("MSG,"); Serial.println(txmessage);
	return 0;
}

// CALL Command
// Limits the callsign to 6 characters.
int parseCallsign(String input){
	input.toCharArray(callsign,7);
	store_callsign(callsign);
	Serial.print("CALL,"); Serial.println(callsign);
	return 0;
}

// IDENT Command
void ident(){
	//Serial.print("Sending Ident: DE ");
	//Serial.println(callsign);
	morse_set_wpm(20);
	tx_on();
	morse_tx_string("DE ");
	morse_tx_string(callsign);
	tx_off();
}

// SELCALL Command
int parseSelcall(String source, String dest){
	unsigned int sourcecall = 0;
	unsigned int destcall = 0;
	char* endptr;
	
	// Attempt to convert source ID
	source.toCharArray(tempBuffer,10);
	sourcecall = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(sourcecall>9999) return -1;
	
	// Attempt to convert destination ID
	dest.toCharArray(tempBuffer,10);
	destcall = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(destcall>9999) return -1;
	
	selcall_setup(sourcecall,tx_sideband);
	selcall_call(destcall);
	return 0;
}

// SELTEST Command
int parseSeltest(String source, String dest){
	unsigned int sourcecall = 0;
	unsigned int destcall = 0;
	char* endptr;
	
	// Attempt to convert source ID
	source.toCharArray(tempBuffer,10);
	sourcecall = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(sourcecall>9999) return -1;
	
	// Attempt to convert destination ID
	dest.toCharArray(tempBuffer,10);
	destcall = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(destcall>9999) return -1;
	
	selcall_setup(sourcecall,tx_sideband);
	selcall_chan_test(destcall);
	return 0;
}

// RTTY Command
int parseRTTY(String baud, String shift){
	unsigned int baudrate = 0;
	unsigned int shift_freq = 0;
	char* endptr;
	
	// Attempt to convert source ID
	baud.toCharArray(tempBuffer,10);
	baudrate = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(baudrate>600) return -1;
	
	// Attempt to convert destination ID
	shift.toCharArray(tempBuffer,10);
	shift_freq = (unsigned int)strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	if(shift_freq>3000) return -1;
	
	//rtty_set_mode(baudrate,shift_freq,8);
	//rtty_tx_string(txmessage);
	rtty_start(baudrate,shift_freq);
	store_string(txmessage);
	return 0;
}

// Button Handling Functions

void button_isr(){
    button_state = 1;
    digitalWrite(LED_PIN, HIGH);
}

void read_inhibit(){
    Serial.print("INHIBIT,");
    if(button_state) Serial.println("ON");
    else    Serial.println("OFF");
}

int write_inhibit(String input){
	if(input.startsWith("ON")){
		button_state = 1;
		Serial.println("INHIBIT,ON");
	}else if(input.startsWith("OFF")){
		button_state = 0;
		digitalWrite(LED_PIN, LOW);
		Serial.println("INHIBIT,OFF");
	}else{
		return -1;
	}
	return 0;
}

void transmit_rtty(){
	rtty_start(50,170);
	delay(3000);
	store_string(txmessage);
	while(data_waiting(&data_tx_buffer)>0);
	delay(3000);
}

void selcall_test(){
	selcall_setup(1234,1);
	selcall_chan_test(1882);
}

void transmit_psk(){
	bpsk_start(31);
	delay(3000);
	bpsk_add_data(txmessage);
	while(data_waiting(&data_tx_buffer)>0);
	delay(3000);
	bpsk_add_data(txmessage);
	while(data_waiting(&data_tx_buffer)>0);
	delay(3000);
	bpsk_stop();
}
