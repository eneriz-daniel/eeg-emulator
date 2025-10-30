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
//  ADS1299Manager.h
//  Part of the Arduino Library for the ADS1299 Shield
//  Created by Chip Audette, Fall 2013
//
//  Modified by Daniel Enériz Orta through March 2024
//  	- Use only ADS1299 functionalities for our project



#ifndef ____ADS1299Manager__
#define ____ADS1299Manager__

#include <ADS1299.h>

#include <Commands.h>

#define NCHAN_PER_BOARD (8)  // number of EEG channels

/*   Arduino Uno - Pin Assignments
  SCK = 13
  MISO [DOUT] = 12
  MOSI [DIN] = 11
  CS = 10; 
  RESET = 9;
  DRDY = 8;
*/

#define PIN_DRDY (8)
#define PIN_RST (9)
#define PIN_CS (10)
#define SCK_MHZ (4)

//gainCode choices
#define ADS_GAIN01 (0b00000000)
#define ADS_GAIN02 (0b00010000)
#define ADS_GAIN04 (0b00100000)
#define ADS_GAIN06 (0b00110000)
#define ADS_GAIN08 (0b01000000)
#define ADS_GAIN12 (0b01010000)
#define ADS_GAIN24 (0b01100000)
#define ADS_GAIN00 (0b01110000) // This does not use the PGA

// dataRateCode choices
#define ADS_DR_16K (0b00000000)
#define ADS_DR_8K (0b00000001)
#define ADS_DR_4K (0b00000010)
#define ADS_DR_2K (0b00000011)
#define ADS_DR_1K (0b00000100)
#define ADS_DR_500 (0b00000101)
#define ADS_DR_250 (0b00000110)

//inputCode choices
#define ADSINPUT_NORMAL (0b00000000)
#define ADSINPUT_SHORTED (0b00000001)
#define ADSINPUT_TESTSIG (0b00000101)
#define ADSINPUT_BIAS_MEAS (0b00000010)

//test signal choices...ADS1299 datasheet page 41
#define ADSTESTSIG_AMP_1X (0b00000000)
#define ADSTESTSIG_AMP_2X (0b00000100)
#define ADSTESTSIG_PULSE_SLOW (0b00000000)
#define ADSTESTSIG_PULSE_FAST (0b00000001)
#define ADSTESTSIG_DCSIG (0b00000011)
#define ADSTESTSIG_NOCHANGE (0b11111111)

// Output modes
#define OUTPUT_TYPE_TEXT (0)
#define OUTPUT_TYPE_BINARY (1)
#define OUTPUT_TYPE_OPENEEG (2)
#define OUTPUT_TYPE_TELEMETRY (3)

#define OFF (0)
#define ON (1)

//binary communication codes for each packet
#define PCKT_START 0xA0
#define PCKT_END 0xC0

class ADS1299Manager : public ADS1299 {
  public:
    void initialize(bool isDaisy);
    void reset(void);
    void activateInternalReference(void);
    void deactivateChannel(int N_oneRef);
    void deactivateAllChannels(void);
    void enableChannel(int N_oneRef);
    void enableAllChannels(void);
    void setClockOutput(bool enable);
    void configureDataRate(byte dataRateCode);
    void configureChannelGain(int N_oneRef, byte gainCode);
    void configureChannelInput(int N_oneRef, byte inputCode);
    void configureChannelPositiveBias(int N_oneRef, bool enable);
    void configureChannelNegativeBias(int N_oneRef, bool enable);
    void powerBiasDrive(bool enable);
    void configureBiasRefInt(bool enable);
    void setBiasMeas(int N_oneRef);
    void unsetBiasMeas();
    bool isChannelActive(int N_oneRef);
    int getChannelGain(int N_oneRef);
    void configureInternalTestSignal(byte amplitudeCode, byte freqCode);
    void connectSRB1(bool connect);
    void connectSRB2(int N_oneRef, bool connect);
    void startDataContinuous(void);
    void stopDataContinuous(void);
    void startStreaming(void);
    void stopStreaming(void);
    void singleShotMode(bool enableSingleShotMode);
    void singleRead(void);
    int isDataAvailable(void);
    void batchRead(int N);
    void getBatch(void);
    void printChannelDataAsText(int N, long int sampleNumber, bool sendAuxValue, long int auxValue);
    void printChannelDataAsTelemetry(int N, long int sampleNumber, bool sendAuxValue, long int auxValue);
    void writeChannelDataAsBinary(int N, long int sampleNumber);
    void writeChannelDataAsBinary(int N, long int sampleNumber, bool useSyntheticData);
    void writeChannelDataAsBinary(int N, long int sampleNumber, long int auxValue);
    void writeChannelDataAsBinary(int N, long int sampleNumber, long int auxValue, bool useSyntheticData);
    void writeChannelDataAsBinary(int N, long int sampleNumber, bool sendAuxValue,long int auxValue, bool useSyntheticData);
    void writeChannelDataAsOpenEEG_P2(long int sampleNumber);
    void writeChannelDataAsOpenEEG_P2(long int sampleNumber, bool useSyntheticData);
    void printAllRegisters(void);
    void printData();
    void printData(long sampleNumber);
    void printData(long sampleNumber, bool sendAuxValue, long int auxValue);
    void printData(int N, long sampleNumber, bool sendAuxValue, long int auxValue, bool useSyntheticData);
    void printHelp(void);
    void processInputCommands(String command);

    int nChans; // Number of total channels (8 or 16)
    int outputMode; // Output mode (text, binary, openEEG)
    bool isRunning; // True if RDATAC is active, false otherwise
    bool managerVerbose; // True if the manager is verbose, false otherwise
    int batchSize; // Number of samples to read in batch mode
    long batchData[MAX_BATCH_SIZE][16]; // Data buffer for batch reads
    
  private:
    long int makeSyntheticSample(long sampleNumber,int chan);
};

#endif