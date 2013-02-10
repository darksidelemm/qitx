/*
	QITX
	HF Selcall CCIR 493-4 Implementation
	
	This implementation is based partly on the CCIR 493-9 specification,
	and partly on reverse engineered SELCALL packets received from a commercial
	HF radio.
	
	CCIR 493-9 is available here: http://hflink.com/selcall/ITU-R%20M.493-9%20specification.pdf
	
	Public functions from this file:
	void selcall_setup(unsigned int source_addr, int lsb)
	Setup the selcall encoder (set DDS frequency registers, etc)
	source_addr: 4-digit selcall address to be used as the source address.
	lsb: 1 if a LSB channel to be used, 0 if USB.
	
	void selcall_call(unsigned int dest_addr)
	Send a call request to a 4-digit destination address.
	If the destination radio decodes the message, it will ring.
	dest_addr: 4-digit destination address.
	
	void selcall_chan_test(unsigned int dest_addr)
	Perform a channel test using another radio.
	If the destination radio decodes the message, it will respond with a tone sequence.
	dest_addr: 4-digit destination address.
	
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


#define SELCALL_DELAY	9800  // 100 baud on an ATMega32U4 @ 16MHz

// Some defines for SELCALL Codes.
#define SEL_SEL	120		// Selective call
#define SEL_ID	123		// Individual station semi-automatic/automatic service (Codan channel test)
#define SEL_EOS	127		// ROS
#define SEL_RTN	100		// Routine call
#define SEL_ARQ	117		// Acknowledge Request (EOS)
#define SEL_PDX	125		// Phasing DX Position
#define SEL_PH7	111		// Phasing RX-7 position
#define SEL_PH6	110		// RX-6
#define SEL_PH5	109		// RX-5
#define SEL_PH4	108		// RX-4
#define SEL_PH3	107		// RX-3
#define SEL_PH2	106		// RX-2
#define SEL_PH1	105		// RX-1
#define SEL_PH0	104		// Phasing RX-0 Position

unsigned int selcall_src = 0; // Source address.

void selcall_setup(unsigned int source_addr, int usb){
	selcall_src = source_addr;


	// Setup our transmit frequency registers.
	// This assumes tx_freq is set to the suppressed carrier frequency of our channel.
	// The standard calls for our tones to be at 1700 and 1870 Hz within the channel.
	if(usb){
		AD9834_SetFreq(0,tx_freq + 1700);
		AD9834_SetFreq(1,tx_freq + 1870);
		
	}else{
		AD9834_SetFreq(0,tx_freq - 1700);
		AD9834_SetFreq(1,tx_freq - 1870);
		
	}
	AD9834_SelectFREG(0);
}

void selcall_send_message(int* msg, int msglen){

/*  // ECC Calculations not required for CCIR 493-4.
	// Calculate the ECC first. ITU-R M.493-9 part 10.2
	uint16_t ecc = 0;
	
	for(int k = 0; k<(msglen/2 -2); k++){
		ecc = ecc^msg[k*2];
	}
	ecc = ecc & 0x007F;
*/

	tx_on();
	// Send preamble for 6 seconds.	
	for(unsigned int k=0; k<700; k++){
		AD9834_SelectFREG(0);
		delayMicroseconds(SELCALL_DELAY);
		AD9834_SelectFREG(1);
		delayMicroseconds(SELCALL_DELAY);
	}
	// Send Phasing pattern.
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH5); // PH5
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH4); // PH4
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH3); // PH3
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH2); // PH2
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH1); // PH1
	selcall_send_word(SEL_PDX); // PDX
	selcall_send_word(SEL_PH0); // PH0
	
	for(int k = 0; k<msglen; k++){
		selcall_send_word(msg[k]);
	}
	tx_off();
	
}


void selcall_send_word(int symb){
	if(symb<128){
		uint16_t current_word = selcall_get_word((unsigned int) symb);
		for(int i = 0; i<10; i++){
			if(current_word&1){
				AD9834_SelectFREG(1);
			}else{
				AD9834_SelectFREG(0);
			}
			current_word = current_word>>1;
			delayMicroseconds(SELCALL_DELAY);
		}
	}
}

// A CCIR 493-4 word consists of a 7-bit word number (0-127), and a 3-bit parity,
// which is the number of 0's in the word.
// The 7-bit word number is send LSB first, then the parity MSB first.
uint16_t selcall_get_word(uint16_t value){
	
	uint16_t accum = 0;
	uint16_t lookuptab[] = {0x0000,0x0200,0x0100,0x0300,0x0080,0x0280,0x0180,0x0380};
	for(int i = 0; i<7; i++){
		if(!((value>>i)&1)) accum++;
	}
	return (value&0x007F)|lookuptab[accum&0x007F];
	
}

// Call a 4-digit destination address.
// On most commercial HF radios (Codan, Barrett), this will make the radio ring.
void selcall_call(unsigned int dest_addr){
	int addr_A1 = (selcall_src/100)%100;
	int addr_A2 = selcall_src%100;
	int addr_B1 = (dest_addr/100)%100;
	int addr_B2 = dest_addr%100;

	int callmsg[] = {
		SEL_SEL, // DX 
		SEL_SEL, // RX
		addr_B1, // DX
		SEL_SEL, // RX
		addr_B2, // DX
		SEL_SEL, // RX
		SEL_RTN, // DX
		addr_B1, // RX
		addr_A1, // DX
		addr_B2, // RX
		addr_A2, // DX
		SEL_RTN, // RX
		SEL_ARQ, // DX
		addr_A1, // RX
		SEL_ARQ, // DX
		addr_A2, // RX
		SEL_ARQ, // DX
		SEL_ARQ, // RX
	};
	
	selcall_send_message(callmsg,18);
}

// Perform a channel test to a 4-digit selcall address, which causes the recepient
// radio to respond with a 'High-Low-Low-Low-Low' tone sequence.
// This is only confirmed to work with Codan radios.
void selcall_chan_test(unsigned int dest_addr){
	int addr_A1 = (selcall_src/100)%100;
	int addr_A2 = selcall_src%100;
	int addr_B1 = (dest_addr/100)%100;
	int addr_B2 = dest_addr%100;

	int callmsg[] = {
		SEL_ID,  // DX 
		SEL_ID,  // RX
		addr_B1, // DX
		SEL_ID,  // RX
		addr_B2, // DX
		SEL_ID,  // RX
		SEL_RTN, // DX
		addr_B1, // RX
		addr_A1, // DX
		addr_B2, // RX
		addr_A2, // DX
		SEL_RTN, // RX
		SEL_ARQ, // DX
		addr_A1, // RX
		SEL_ARQ, // DX
		addr_A2, // RX
		SEL_ARQ, // DX
		SEL_ARQ, // RX
	};
	
	selcall_send_message(callmsg,18);
}

