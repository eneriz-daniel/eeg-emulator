// The modifications made by Daniel Enériz are licensed under the GNU General Public License v3.0.
//
// Copyright (C) 2024 Daniel Enériz
// 
// This file is part of ADS1299 driver.
// 
// ADS1299 driver is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// ADS1299 driver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with ADS1299 driver. If not, see <http://www.gnu.org/licenses/>.
//
//
//  ADS1299DIAISY.cpp   ARDUINO LIBRARY FOR COMMUNICATING WITH TWO
//  DAISY-CHAINED ADS1299 BOARDS
//  
//  Created by Conor Russomanno, Luke Travis, and Joel Murphy. Summer, 2013
//
//  Extended by Chip Audette through April 2014
//
//  Modified by Indrayudd Roy Chowdhury through March 2023
//
//  Modified by Daniel Enériz Orta through January 2024
//  	- Use the Arduino SPI library
//  	- Documentation



//#include "pins_arduino.h"
#include "ADS1299.h"


/*
* ADS1299 class constructor. Initializes ADS1299 object.
* 
* Arguments:
* 	_DRDY (int): Arduino pin number connected to ADS1299 DRDY pin.
* 	_RST (int): Arduino pin number connected to ADS1299 RST pin.
* 	_CS (int): Arduino pin number connected to ADS1299 CS pin.
* 	_FREQ (int): SPI frequency to communicate at in MHz
* 	_isDaisy (boolean): Boolean value describing whether or not this is a daisy
*   	ADS1299 board.
*
* Returns:
* 	none
*/
void ADS1299::initialize(int _DRDY, int _RST, int _CS, int _FREQ, boolean _isDaisy){
	
	isDaisy = _isDaisy;
	DRDY = _DRDY;
	CS = _CS;
	FREQ = _FREQ;
	RST = _RST;
	
	delay(50);				// recommended power up sequence requiers Tpor (~32mS)	
	pinMode(RST,OUTPUT);
	pinMode(RST,LOW);
	delayMicroseconds(4);	// toggle reset pin
	pinMode(RST,HIGH);
	delayMicroseconds(20);	// recommended to wait 18 Tclk before using device (~8uS);
	

    // SPI configuration
	SPI.begin();
	ADS1299SPISettings = SPISettings(FREQ*1000000ul, MSBFIRST, SPI_MODE2);
 
    // initalize the  data ready chip select and reset pins:
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT);

	// Set the number of channels
	if (isDaisy){
		N_CHANS = 16;
	}	
	else{
		N_CHANS = 8;
	}
		
	digitalWrite(CS,HIGH);
	digitalWrite(RST,HIGH);
}

//SYSTEM COMMANDS

// Get out of low power mode. Only allowed to send WAKEUP after sending STANDBY
void ADS1299::WAKEUP() { 
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW);					  // Open SPI
    SPI.transfer(_WAKEUP);                    // Send WAKEUP command to the ADS1299
	digitalWrite(CS,HIGH);		              // Close SPI
    SPI.endTransaction(); 
    delayMicroseconds(3);  		              // must wait 4 tCLK cycles before
											  // sending another command
											  // (Datasheet, pg. 40)
}

// Go into low power mode
void ADS1299::STANDBY() {		
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW);					  // Open SPI
    SPI.transfer(_STANDBY);                   // Send STANDBY command to the ADS1299
	digitalWrite(CS,HIGH);		              // Close SPI
    SPI.endTransaction(); 
}

// Reset all the registers to default settings
void ADS1299::RESET() {			
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW);					  // Open SPI
    SPI.transfer(_RESET);                     // Send RESET command to the ADS1299
	delayMicroseconds(12);   				  // must wait 18 tCLK cycles to
											  // execute this command
											  // (Datasheet, pg. 41)
	digitalWrite(CS,HIGH);		              // Close SPI
    SPI.endTransaction(); 
}

// Start data conversion 
void ADS1299::START() {			
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW);					  // Open SPI
    SPI.transfer(_START);                     // Send START command to the ADS1299
	digitalWrite(CS,HIGH);		              // Close SPI
    SPI.endTransaction(); 
}

// Stop data conversion
void ADS1299::STOP() {			
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW);					  // Open SPI
    SPI.transfer(_STOP);                      // Send STOP command to the ADS1299
	digitalWrite(CS,HIGH);		              // Close SPI
    SPI.endTransaction();
}

// Enable Read Data Continuous mode
void ADS1299::RDATAC() {
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
    digitalWrite(CS,LOW); 				      // Open SPI
    SPI.transfer(_RDATAC); 				      // Send RDATAC command to the ADS1299
	digitalWrite(CS,HIGH); 		              // Close SPI
    SPI.endTransaction();
	delayMicroseconds(3);                     // must wait 4 tCLK cycles after
	                                          // executing this command
											  // (Datasheet, pg. 41)
}

// Disable Read Data Continuous mode
void ADS1299::SDATAC() {
    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
    digitalWrite(CS,LOW); 				      // Open SPI
    SPI.transfer(_SDATAC); 				      // Send SDATAC command to the ADS1299
	digitalWrite(CS,HIGH); 		              // Close SPI
    SPI.endTransaction();
	delayMicroseconds(3);                     // must wait 4 tCLK cycles after
	                                          // executing this command
											  // (Datasheet, pg. 42)
}

// Get Device ID (Datasheet, pg. 45)
byte ADS1299::getDeviceID() {
	byte data = RREG(0x00);                   // Read register 0x00 (Datasheet,
	                                          // Table 11, pg. 44)
	if(verbose){							  // Verbose output
		Serial.print(F("Device ID "));
		printHex(data);	
	}
	return data;
}

/*
* Read one ADS1299 register at the given address and updates the mirror array,
* regData.
*
* Arguments:
* 	_address (byte): The address of the register to read.
* 
* Returns:
* 	The value of the register at the given address.
*/
byte ADS1299::RREG(byte _address) {
    byte opcode1 = _address + 0x20; 	      // RREG expects 001rrrrr where
	   									      // rrrrr = _address (Datahseet,
											  // pg. 43)

    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI

    SPI.transfer(opcode1); 					  // Send first command byte, RREG
	                                          // header and address
	
    SPI.transfer(0x00); 					  // Send second command byte, number
	                                          // of registers to read -1. In this
											  // case, the number of registers is
											  // just 1, so we send 0x00.
											  
    regData[_address] = SPI.transfer(0x00);   // Read register value and update
	                                          // mirror location with returned byte

	digitalWrite(CS,HIGH); 		              // Close SPI
    SPI.endTransaction();

	if (verbose){						      // Verbose output
		printRegisterName(_address);
		printHex(_address);
		Serial.print(", ");
		printHex(regData[_address]);
		Serial.print(", ");
		for(byte j = 0; j<8; j++){
			Serial.print(bitRead(regData[_address], 7-j));
			if(j!=7) Serial.print(", ");
		}
		
		Serial.println();
	}
	return regData[_address];			      // Return requested register value
}

/*
* Read multiple ADS1299 registers starting at the given address and updates the
* mirror array, regData.
*
* Arguments:
* 	_address (byte): The address of the first register to read.
* 	_numRegistersMinusOne (byte): The number of registers to read minus one.
*
* Returns:
* 	none
*/
void ADS1299::RREGS(byte _address, byte _numRegistersMinusOne) {
//	for(byte i = 0; i < 0x17; i++){           // Clear the regData array
//		regData[i] = 0;
//	}
    byte opcode1 = _address + 0x20; 	      // RREG expects 001rrrrr where
	   									      // rrrrr = _address (Datahseet,
											  // pg. 43)

    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI
    
    SPI.transfer(opcode1); 					  // Send first command byte, RREG
	                                          // header and address

    SPI.transfer(_numRegistersMinusOne);	  // Send second command byte, number
	                                          // of registers to read -1. In this
											  // case, the number of registers is
											  // _numRegistersMinusOne + 1, so
											  // we send _numRegistersMinusOne.

    for(int i = 0; i <= _numRegistersMinusOne; i++){
        regData[_address + i] = SPI.transfer(0x00); // Read register value and update
	}                                               // mirror location with returned byte
	
	digitalWrite(CS,HIGH); 		              // Close SPI
    SPI.endTransaction();
	
	if(verbose){						      // Verbose output
		for(int i = 0; i<= _numRegistersMinusOne; i++){
			printRegisterName(_address + i);
			printHex(_address + i);
			Serial.print(", ");
			printHex(regData[_address + i]);
			Serial.print(", ");
			for(int j = 0; j<8; j++){
				Serial.print(bitRead(regData[_address + i], 7-j));
				if(j!=7) Serial.print(", ");
			}
			Serial.println();
		}
    }  
}

/*
* Write one ADS1299 register at the given address and updates the mirror array,
* regData.
*
* Arguments:
* 	_address (byte): The address of the register to write to.
* 	_value (byte): The value to write to the register.
*
* Returns:
* 	none
*/
void ADS1299::WREG(byte _address, byte _value) {
    byte opcode1 = _address + 0x40; 	      // WREG expects 010rrrrr where
	   									      //  rrrrr = _address (Datasheet,
											  //  pg. 43)

    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI
    
    SPI.transfer(opcode1);					  // Send first command byte, WREG
											  // header and _address

    SPI.transfer(0x00);						  // Send second command byte, number
											  // of registers to write -1. In this
											  // case, the number of registers is
											  // just 1, so we send 0x00.

    SPI.transfer(_value);					  // Send the value to write to the
											  // register

	digitalWrite(CS,HIGH); 		              // Close SPI
	SPI.endTransaction();

	regData[_address] = _value;			      // Update the mirror array.
											  // Though it shouldn't be necessary,
											  // we could update the shadow array
											  // here as well.

	if(verbose){						      // Verbose output
		Serial.print(F("Register "));
		printHex(_address);
		Serial.println(F(" modified."));
	}
}

/*
* Write multiple ADS1299 registers starting at the given address using the
* values stored in the mirror array, regData.
*
* Arguments:
* 	_address (byte): The address of the first register to write to.
* 	_numRegistersMinusOne (byte): The number of registers to write minus one.
*
* Returns:
* 	none
*/
void ADS1299::WREGS(byte _address, byte _numRegistersMinusOne) {
    byte opcode1 = _address + 0x40;		      // WREG expects 010rrrrr where
	   									      // rrrrr = _address (Datasheet,
											  // pg. 43)

    SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI
    
    SPI.transfer(opcode1);					  // Send first command byte, WREG
											  // header and _address

    SPI.transfer(_numRegistersMinusOne);	  // Send second command byte, number
											  // of registers to write -1. In this
											  // case, the number of registers is
											  // _numRegistersMinusOne + 1, so
											  // we send _numRegistersMinusOne.
											  
	for (int i=_address; i <=(_address + _numRegistersMinusOne); i++){
		SPI.transfer(regData[i]);			  // Write to the registers
	}

	digitalWrite(CS,HIGH);				      // Close SPI
	SPI.endTransaction();

	if(verbose){  						      // Verbose output
		Serial.print(F("Registers "));
		printHex(_address); Serial.print(F(" to "));
		printHex(_address + _numRegistersMinusOne);
		Serial.println(F(" modified"));
	}
}
/*
* This function reads the ADS1299's channels data registers and stores the
* results in the channelData array during RDATAC mode (Read Data Continuous mode).
* 
* It is coded assuming that the ADS1299 has 8 channels. Additionally it supports
* daisy chain mode, with up to 16 channels (two ADS1299 chips).
*
* TODO: Update this to support 4- and 6-channel ADS1299 chips and an arbitrary
* number of daisy-chained ADS1299 chips.
*
* Arguments:
* 	none
*
* Returns:
* 	none
*
*/
void ADS1299::updateChannelData(){
	byte inByte;                              // Variable to store incoming byte

	int nchan=8;                              // Assume 8-channel ADS1299. Daisy
	                                          // chain functionality (for 16
											  // channels) is implemented in
											  // block below.

	SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI
	
	// Read channel data from ADS1299 (ADS 1)
    
	for(int i=0; i<3; i++){			          // Read 3 byte status register
	    inByte = SPI.transfer(0x00);          // from ADS 1 (1100 + LOFF_STATP +
		stat_1 = (stat_1<<8) | inByte;		  // LOFF_STATN+GPIO[7:4])
	}										  // (Datasheet, pg. 36)

	
	for(int i = 0; i<8; i++){                 // Iterate over the 8 channels

		for(int j=0; j<3; j++){		          // Read 24 bits of channel data 
			inByte = SPI.transfer(0x00);      // from ADS 1 in 3 byte chunks
			channelData[i] = (channelData[i]<<8) | inByte;
		}
	}

	// If daisy chain mode is enabled, read data from ADS 2 as well
	
	if (isDaisy) {
		nchan = 16;                           // Update the number of channels
		                                      // for future use

		for(int i=0; i<3; i++){			      // Read 3 byte status register
			inByte = SPI.transfer(0x00);      // from ADS 2 (1100 + LOFF_STATP +
			stat_2 = (stat_2<<8) | inByte;    // LOFF_STATN+GPIO[7:4])
		}                                     // (Datasheet, pg. 36)
		
		for(int i = 8; i<16; i++){            // Iterate over the 8 channels

			for(int j=0; j<3; j++){		      // Read 24 bits of channel data
				inByte = SPI.transfer(0x00);  // from ADS 2 in 3 byte chunks
				channelData[i] = (channelData[i]<<8) | inByte;
			}
		}
	}
	
	digitalWrite(CS,HIGH);				      // Close SPI
	SPI.endTransaction();
	
	// Reformat the numbers
	for(int i=0; i<nchan; i++){			      // Convert 3 byte 2's compliment 
		if(bitRead(channelData[i],23) == 1){  // to 4 byte 2's compliment
			channelData[i] |= 0xFF000000;
		}
		else{
			channelData[i] &= 0x00FFFFFF;
		}
	}
}

/*
* This function reads the ADS1299's channels data registers and stores the
* results in the channelData using RDATA command (single read). It must be used
* outside the RDATAC mode (Read Data Continuous mode) (use SDATAC to exit
* RDATAC), and after DRDY goes low.
*
* It is coded assuming that the ADS1299 has 8 channels. Additionally it supports
* daisy chain mode, with up to 16 channels (two ADS1299 chips).
*
* TODO: Update this to support 4- and 6-channel ADS1299 chips and an arbitrary
* number of daisy-chained ADS1299 chips.
*
* Arguments:
* 	none
*
* Returns:
* 	none
*
*/
void ADS1299::RDATA() {
	byte inByte; 							  // Variable to hold the incoming byte
	stat_1 = 0;							      // Clear status registers
	stat_2 = 0;	
	int nchan=8;                              // Assume 8-channel ADS1299. Daisy
	                                          // chain functionality (for 16
											  // channels) is implemented in
											  // block below.

	SPI.beginTransaction(ADS1299SPISettings); // Configure SPI settings
	digitalWrite(CS,LOW); 				      // Open SPI

	SPI.transfer(_RDATA);                     // Send RDATA command to the ADS1299
	
	// Read channel data from ADS1299 (ADS 1)
	for(int i=0; i<3; i++){			 		  // Read 3 byte status register
	    inByte = SPI.transfer(0x00);          // from ADS 1 (1100 + LOFF_STATP +
		stat_1 = (stat_1<<8) | inByte;	      // LOFF_STATN+GPIO[7:4])			
	} 									      // (Datasheet, pg. 36)
	
	for(int i = 0; i<8; i++){                 // Iterate over the 8 channels

		for(int j=0; j<3; j++){		 		  // Read 24 bits of channel data
			inByte = SPI.transfer(0x00); 	  // from ADS 1 in 3 byte chunks
			channelData[i] = (channelData[i]<<8) | inByte;
		}
	}
	
	// If daisy chain mode is enabled, read data from ADS 2 as well
	if (isDaisy) {
		nchan = 16; 						  // Update the number of channels
		                                      // for future use
		
		for(int i=0; i<3; i++){				  // Read 3 byte status register
			inByte = SPI.transfer(0x00); 	  // from ADS 2 (1100 + LOFF_STATP +
			stat_2 = (stat_2<<8) | inByte;	  // LOFF_STATN+GPIO[7:4])			
		} 									  // (Datasheet, pg. 36)
		
		for(int i = 8; i<16; i++){ 		      // Iterate over the 8 channels
			for(int j=0; j<3; j++){		      // Read 24 bits of channel data
				inByte = SPI.transfer(0x00);  // from ADS 2 in 3 byte chunks
				channelData[i] = (channelData[i]<<8) | inByte;
			}
		}
	}
	
	digitalWrite(CS,HIGH);				      // Close SPI
	SPI.endTransaction();

	// Reformat the numbers
	for(int i=0; i<nchan; i++){			      // Convert 3 byte 2's compliment
		if(bitRead(channelData[i],23) == 1){  // to 4 byte 2's compliment
			channelData[i] |= 0xFF000000;
		}
		else{
			channelData[i] &= 0x00FFFFFF;
		}
	}
}

/*
* This function prints the name of the given register to the serial monitor.
*
* Arguments:
* 	_address (byte): The address of the register to print the name of.
*
* Returns:
* 	none
*
*/
void ADS1299::printRegisterName(byte _address) {
	switch (_address) {
		case ID:
			Serial.print(F("ID, "));
			break;
		case CONFIG1:
			Serial.print(F("CONFIG1, "));
			break;
		case CONFIG2:
			Serial.print(F("CONFIG2, "));
			break;
		case CONFIG3:
			Serial.print(F("CONFIG3, "));
			break;
		case LOFF:
			Serial.print(F("LOFF, "));
			break;
		case CH1SET:
			Serial.print(F("CH1SET, "));
			break;
		case CH2SET:
			Serial.print(F("CH2SET, "));
			break;
		case CH3SET:
			Serial.print(F("CH3SET, "));
			break;
		case CH4SET:
			Serial.print(F("CH4SET, "));
			break;
		case CH5SET:
			Serial.print(F("CH5SET, "));
			break;
		case CH6SET:
			Serial.print(F("CH6SET, "));
			break;
		case CH7SET:
			Serial.print(F("CH7SET, "));
			break;
		case CH8SET:
			Serial.print(F("CH8SET, "));
			break;
		case BIAS_SENSP:
			Serial.print(F("BIAS_SENSP, "));
			break;
		case BIAS_SENSN:
			Serial.print(F("BIAS_SENSN, "));
			break;
		case LOFF_SENSP:
			Serial.print(F("LOFF_SENSP, "));
			break;
		case LOFF_SENSN:
			Serial.print(F("LOFF_SENSN, "));
			break;
		case LOFF_FLIP:
			Serial.print(F("LOFF_FLIP, "));
			break;
		case LOFF_STATP:
			Serial.print(F("LOFF_STATP, "));
			break;
		case LOFF_STATN:
			Serial.print(F("LOFF_STATN, "));
			break;
		case GPIO:
			Serial.print(F("GPIO, "));
			break;
		case MISC1:
			Serial.print(F("MISC1, "));
			break;
		case MISC2:
			Serial.print(F("MISC2, "));
			break;
		case CONFIG4:
			Serial.print(F("CONFIG4, "));
			break;
	}
}

/*
* This function prints the given byte in HEX to the serial monitor.
*
* Arguments:
* 	_data (byte): The byte to print in HEX.
*
* Returns:
* 	none
*
*/
void ADS1299::printHex(byte _data){
	Serial.print("0x");
    if(_data < 0x10) Serial.print("0");
    Serial.print(_data, HEX);
}
