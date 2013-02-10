/*
	Multi-mode HF data software for Arduino + AD9834 DDS.
	
	PSK Module.
	
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
long rtty_delay = 20000;
unsigned int rtty_bits = 8;
unsigned int rtty_shift = 300;
int rtty_ASCII_LENGTH = 7;

/*
void rtty_set_mode (unsigned int baud_rate, unsigned int shift, unsigned int bits){
	
	rtty_ASCII_LENGTH = bits;

	// Configure the frequency registers for the correct shift.
	rtty_shift = shift;
	if(tx_sideband){ 
		//USB
		AD9834_SetFreq(0,tx_freq + tx_offset);
		AD9834_SetFreq(1,tx_freq + tx_offset + shift);
	}else{
		//LSB
		AD9834_SetFreq(0,tx_freq - tx_offset);
		AD9834_SetFreq(1,tx_freq - tx_offset - shift);
	}
	AD9834_SelectFREG(0);
	//AD9834_Use_Pins(1);
	
	// Tweak timing
	switch(baud_rate){
		case 45:
			rtty_delay = 22222;
			break;
		case 50:
			rtty_delay = 20000;
			break;
		case 75:
			rtty_delay = 13333;
			break;
		case 100:
			rtty_delay = 10000;
			break;
		case 300:
			rtty_delay = 3333;
			break;
		case 600:
			rtty_delay = 1666;
		default: // Default to 50 baud
			rtty_delay = 20000;
			break;
	}

}

char rtty_txBuffer[100];

volatile boolean rtty_txLock = false;
volatile int rtty_current_tx_byte = 0;
volatile short rtty_current_byte_position;
// Always useful to have an interrupt counter.
volatile int rtty_interruptCount = 0;

void rtty_tx_interrupt(){
  rtty_interruptCount++;
  if(rtty_txLock){
          
      // Pull out current byte
      char current_byte = rtty_txBuffer[rtty_current_tx_byte];
      
      // Null character? Finish transmitting
      if(current_byte == 0){
         rtty_txLock = false;
         return;
      }
      
      int rtty_current_bit = 0;
      
      if(rtty_current_byte_position == 0){ // Start bit
          rtty_current_bit = 0;
      }else if(rtty_current_byte_position == (rtty_ASCII_LENGTH + 1)){ // Stop bit
          rtty_current_bit = 1;
      }else{ // Data bit
       rtty_current_bit = 1&(current_byte>>(rtty_current_byte_position-1));
      }
      
      
      // Transmit!
      rtty_tx_bit(rtty_current_bit);
      
      // Increment all our counters.
      rtty_current_byte_position++;
      
      if(rtty_current_byte_position==(rtty_ASCII_LENGTH + 2)){
          rtty_current_tx_byte++;
          rtty_current_byte_position = 0;
      }
  }
}

void rtty_tx_string(char *string){
  if(rtty_txLock == false){
    strcpy(rtty_txBuffer, string);
    rtty_current_tx_byte = 0;
    rtty_current_byte_position = 0;
    rtty_txLock = true;
  }
  tx_on();
  Timer1.initialize(rtty_delay);
  Timer1.attachInterrupt(rtty_tx_interrupt);
  rtty_waitForTX();
  Timer1.detachInterrupt();
  tx_off();
}

void rtty_waitForTX(){
    while(rtty_txLock){}
}

*/
// New Interrupt-driven RTTY Functions, using ring buffer

unsigned char rtty_current_byte = 0;
uint8_t rtty_byte_position = 10;
uint8_t rtty_current_bit = 1;

void rtty_isr(){
	// First thing first, get the current bit onto the air.
	rtty_tx_bit(rtty_current_bit);
	
	// Now take a look at where we are.
	if(rtty_byte_position == 10){
		if(data_waiting(&data_tx_buffer)>0){
			rtty_current_byte = read_char(&data_tx_buffer);
			rtty_byte_position = 0;
		}else{
			rtty_current_bit= 1;
		}
	}
	
	if(rtty_byte_position == 0){
		rtty_current_bit = 0; // Start bit
		rtty_byte_position++;
	}else if(rtty_byte_position==9){
		rtty_current_bit = 1; // Stop bit
		rtty_byte_position++;
	}else if( (rtty_byte_position < 9) && (rtty_byte_position>0) ){ // Data bit
		rtty_current_bit = rtty_current_byte & 1;
		rtty_current_byte >>= 1;
		rtty_byte_position++;
	}
}

void rtty_start(unsigned int baud_rate, unsigned int shift){
	// Configure the frequency registers for the correct shift.
	rtty_shift = shift;
	if(tx_sideband){ 
		//USB
		AD9834_SetFreq(0,tx_freq + tx_offset);
		AD9834_SetFreq(1,tx_freq + tx_offset + shift);
	}else{
		//LSB
		AD9834_SetFreq(0,tx_freq - tx_offset);
		AD9834_SetFreq(1,tx_freq - tx_offset - shift);
	}
	AD9834_SelectFREG(1);
	//AD9834_Use_Pins(1);
	
	// Tweak timing
	switch(baud_rate){
		case 45:
			rtty_delay = 22222;
			break;
		case 50:
			rtty_delay = 20000;
			break;
		case 75:
			rtty_delay = 13333;
			break;
		case 100:
			rtty_delay = 10000;
			break;
		case 300:
			rtty_delay = 3333;
			break;
		case 600:
			rtty_delay = 1666;
		default: // Default to 50 baud
			rtty_delay = 20000;
			break;
	}
	
	tx_on();
	Timer1.initialize(rtty_delay);
	Timer1.attachInterrupt(rtty_isr);
}

void rtty_stop(){
	tx_off();
	Timer1.detachInterrupt();
	rtty_current_byte = 0;
	rtty_byte_position = 10;
	rtty_current_bit = 1;
}

void rtty_tx_bit (int bit)
{
		if (bit)
		{
			// high
        	AD9834_SelectFREG(1);  
		}
		else
		{
			// low
        	AD9834_SelectFREG(0);
		}
;
}
