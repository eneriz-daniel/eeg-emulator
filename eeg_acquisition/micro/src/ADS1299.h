// The modifications made by Daniel Enériz are licensed under the GNU General
// Public License v3.0.
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
// along with ADS1299 driver.  If not, see <http://www.gnu.org/licenses/>.
//
//
//  ADS1299.h
//  Part of the Arduino Library
//  Created by Conor Russomanno, Luke Travis, and Joel Murphy. Summer 2013.
//
//  Modified by Chip Audette through April 2014
//
//  Modified by Indrayudd Roy Chowdhury through March 2023
//
//  Modified by Daniel Enériz Orta through January 2024
//      - Use of the Arduino SPI library

#ifndef ____ADS1299__
#define ____ADS1299__

#include <stdio.h>
// #include <Arduino.h>
#include <avr/pgmspace.h>
#include "Definitions.h"
#include <SPI.h>


class ADS1299 {
public:
    
    void initialize(int _DRDY, int _RST, int _CS, int _FREQ, boolean _isDaisy);
    
    //ADS1299 SPI Command Definitions (Datasheet, p35)
    //System Commands
    void WAKEUP();
    void STANDBY();
    void RESET();
    void START();
    void STOP();
    
    //Data Read Commands
    void RDATAC();
    void SDATAC();
    void RDATA();
    
    //Register Read/Write Commands
    byte getDeviceID();
    byte RREG(byte _address);
    void RREGS(byte _address, byte _numRegistersMinusOne);     
    void printRegisterName(byte _address);
    void WREG(byte _address, byte _value); 
    void WREGS(byte _address, byte _numRegistersMinusOne); 
    void printHex(byte _data);
    void updateChannelData();
    
    //SPI Transfer function
    // byte transfer(byte _data);

    //configuration
    int DRDY;      // pin number that DRDY is connected to
    int RST;       // pin number that RESET is connected to
    int CS;        // pin number that CS is connected to
    int FREQ;      // value of SCK frequency in MHz
    // int DIVIDER;		// select SPI SCK frequency
    SPISettings ADS1299SPISettings;
    int stat_1, stat_2;    // used to hold the status register for boards 1 and 2
    byte regData [24];	// array is used to mirror register data
    long channelData [16];	// array used when reading channel data board 1+2
    boolean verbose;		// turn on/off Serial feedback
    boolean isDaisy;		// does this have a daisy chain board?
    int N_CHANS;		// number of channels
    
    
};

#endif
