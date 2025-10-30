
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
//  ADS1299Manager.cpp
//  Part of the Arduino Library for the ADS1299 Shield
//  Created by Chip Audette, Fall 2013
//

#include <ADS1299Manager.h>

typedef long int32;
//typedef byte uint8_t;


/**
 * @brief Initializes the ADS1299Manager object.
 * 
 * This function initializes the ADS1299Manager object by calling the initialize
 * function of the ADS1299 class, setting the verbose flag to false, and resetting
 * the ADS1299 device. It also sets the number of channels for all boards based
 * on the isDaisy parameter.
 * 
 * @param isDaisy A bool value indicating whether the daisy chain is used.
 */
void ADS1299Manager::initialize(bool isDaisy) 
{
    ADS1299::initialize(PIN_DRDY,PIN_RST,PIN_CS,SCK_MHZ,isDaisy); // (DRDY pin, RST pin, CS pin, SCK frequency in MHz);
    delay(100);
        
    verbose = false; // Disable register-level verbosity
	outputMode = OUTPUT_TYPE_TEXT;  //default output mode is text
	isRunning = false;
	managerVerbose = true; // Enable manager-level verbosity
	batchSize = 0; // Initialize batch size to 0

	nChans = NCHAN_PER_BOARD;
    if (isDaisy) nChans = 2*NCHAN_PER_BOARD;

    reset();
};


/**
 * @brief Resets the ADS1299 device and turns off all channels.
 * 
 * This function sends a RESET command to the ADS1299 device to reset all its registers to default values.
 * It then exits the Read Data Continuous mode to allow communication with the ADS1299.
 * After a delay of 100 milliseconds, it turns off all channels by deactivating them.
 * 
 * @note This function assumes that the ADS1299 device is already initialized and connected.
 */
void ADS1299Manager::reset(void)
{
    ADS1299::RESET();             // send RESET command to default all registers
    ADS1299::SDATAC();            // exit Read Data Continuous mode to communicate
                                  // with ADS
	
	if (managerVerbose) {
		Serial.println("Resetting ADS1299");
	}
    
    delay(100);

	activateInternalReference(); // Activate the internal reference

	isRunning = false;

};


/**
 * @brief Activates the internal reference of the ADS1299Manager.
 * 
 * This function sets the CONFIG3 register of the ADS1299 device to enable the
 * internal reference. The internal reference is used as a voltage reference for
 * the ADC conversion.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 */
void ADS1299Manager::activateInternalReference(void) {
    ADS1299::WREG(CONFIG3,0b11100000); delay(1);
	if (managerVerbose) {
		Serial.println("Activating internal reference");
	}
};


/**
 * @brief Deactivates the given channel of the ADS1299 device.
 * 
 * This function deactivates the given channel of the ADS1299 device by setting
 * the left-most bit of the corresponding CHnSET register to 1. This bit is used
 * to shut down the channel.
 * 
 * @param N The channel number to be deactivated.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 * @note The channel number N is one-referenced (i.e., [1...N]), not [0...N-1].
 * @note The channel number N must be between 1 and 8. If the device is
 * daisy-chained, it will deactivate both the N-th channel of the master board
 * and the N-th channel of the slave board. Remember that in a daisy-chained
 * configuration, both boards share the same configuration.
 */
void ADS1299Manager::deactivateChannel(int N)
{
    byte reg, config;
        
    // Verify that the channel number is valid
    if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
    
    // Disable any data collection
    ADS1299::SDATAC(); delay(1);

	isRunning = false;

    // Shut down the channel without modifying the other bits of the CHnSET register
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);
    reg = CH1SET+(byte)N_zeroRef;
    config = ADS1299::RREG(reg); delay(1);
    bitSet(config,7);  //left-most bit (bit 7) = 1, so this shuts down the channel
    ADS1299::WREG(reg,config); delay(1);
};


/**
 * @brief Deactivates all channels of the ADS1299 device.
 * 
 * This function deactivates all channels of the ADS1299 device by calling the
 * deactivateChannel function for each channel.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 */
void ADS1299Manager::deactivateAllChannels(void) {
    for (int chan=1; chan <= NCHAN_PER_BOARD; chan++) {
        deactivateChannel(chan);
    }
};

/**
 * @brief Powers up the given channel of the ADS1299 device.
 * 
 * This function activates the given channel of the ADS1299 device by setting
 * the left-most bit of the corresponding CHnSET register to 0. This bit is used
 * to power up the channel.
 * 
 * @param N The channel number to be activated.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 * @note The channel number N is one-referenced (i.e., [1...N]), not [0...N-1].
 * @note The channel number N must be between 1 and 8. If the device is
 * daisy-chained, it will deactivate both the N-th channel of the master board
 * and the N-th channel of the slave board. Remember that in a daisy-chained
 * configuration, both boards share the same configuration.
 */
void ADS1299Manager::enableChannel(int N)
{
    byte reg, config;
        
    // Check the inputs
    if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
    
    // Disable any data collection
    ADS1299::SDATAC(); delay(1);

    // Read the previous configuration
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);
    reg = CH1SET+(byte)N_zeroRef;
    config = ADS1299::RREG(reg); delay(1);

    // Activate the channel
    bitClear(config,7);  //left-most bit (bit 7) = 0, so this activates the channel

    // Write the new configuration
    ADS1299::WREG(reg,config); delay(1);

	isRunning = false;
};

/**
 * @brief Activate all channels of the ADS1299 device.
 * 
 * This function activates all channels of the ADS1299 device by calling the
 * enableChannel function for each channel.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 */
void ADS1299Manager::enableAllChannels(void) {
    for (int chan=1; chan <= NCHAN_PER_BOARD; chan++) {
        enableChannel(chan);
    }
};

/**
 * @brief Sets the clock output of the ADS1299 device.
 * 
 * @param enable A bool value indicating whether the clock output should be
 * enabled (true) or disabled (false).
 */
void ADS1299Manager::setClockOutput(bool enable) {

	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	byte config = ADS1299::RREG(CONFIG1); delay(1);

	if (enable) bitSet(config,5);
	else bitClear(config,5);

	ADS1299::WREG(CONFIG1,config); delay(1);
};

/**
 * @brief Configures the data rate of the ADS1299 device.
 * 
 * This function configures the data rate of the ADS1299 device by setting the
 * data rate bits, [2:0], of the CONFIG1 register.
 * 
 * @param dataRateCode The data rate code to be set. Available data rate codes
 * have been defined as macros with the format ADS_DR_XXX, where XXX is the data
 * rate value in decimal samples per second.
 */
void ADS1299Manager::configureDataRate(byte dataRateCode) {

	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	byte config = ADS1299::RREG(CONFIG1); delay(1);
	dataRateCode &= 0b00000111; //only the last three bits should be used
	config = (config & 0b11111000) | dataRateCode; // Bitwise AND to clear the data rate bits and Bitwise OR to set the new data rate bits

	// Write the new configuration
	ADS1299::WREG(CONFIG1,config); delay(1);
};

/**
 * @brief Sets the gain of the given channel of the ADS1299 device.
 * 
 * This function sets the gain of the given channel of the ADS1299 device by
 * configuring the gain bits, [6:4], of the corresponding CHnSET register.
 * 
 * @param N The channel number to be configured.
 * @param gainCode The gain code to be set. Available gain codes have been
 * defined as macros with the format ADS_GAINXX, where XX is the gain value in
 * decimal.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 * @note The channel number N is one-referenced (i.e., [1...N]), not [0...N-1].
 * @note The channel number N must be between 1 and 8. If the device is
 * daisy-chained, it will deactivate both the N-th channel of the master board
 * and the N-th channel of the slave board. Remember that in a daisy-chained
 * configuration, both boards share the same configuration.
 */
void ADS1299Manager::configureChannelGain(int N, byte gainCode)
{
    byte reg, config;
        
    // Check the inputs
    if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
    
    // Disable any data collection
    ADS1299::SDATAC(); delay(1);

    // Read the previous configuration
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);
    reg = CH1SET+(byte)N_zeroRef;
    config = ADS1299::RREG(reg); delay(1);

    // Configure the gain
    gainCode = gainCode & 0b01110000;          // Bitwise AND to get just the bits we want and set the rest to zero
    config = (config & 0b10001111) | gainCode; // Bitwise AND to clear the gain bits and Bitwise OR to set the new gain bits

    // Write the new configuration
    ADS1299::WREG(reg,config); delay(1);

	isRunning = false;
};

/**
 * @brief Sets the input configuration of the given channel of the ADS1299 device.
 * 
 * This function sets the input configuration of the given channel of the ADS1299
 * device by configuring the input bits, [2:0], of the corresponding CHnSET,
 * register, which are also called the MUX bits.
 * 
 * @param N Channel number to be configured.
 * @param inputCode Input code to be set. Available input codes have been defined
 * as macros with the format ADSINPUT_XXXX, where XXXX is the input configuration,
 * that can be NORMAL, SHORTED, or TESTSIG.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 * @note The channel number N is one-referenced (i.e., [1...N]), not [0...N-1].
 * @note The channel number N must be between 1 and 8. If the device is
 * daisy-chained, it will deactivate both the N-th channel of the master board
 * and the N-th channel of the slave board. Remember that in a daisy-chained
 * configuration, both boards share the same configuration.
 */
void ADS1299Manager::configureChannelInput(int N, byte inputCode)
{
    byte reg, config;
        
    // Check the inputs
    if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
    
    // Disable any data collection
    ADS1299::SDATAC(); delay(1);

    // Read the previous configuration
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);
    reg = CH1SET+(byte)N_zeroRef;
    config = ADS1299::RREG(reg); delay(1);

    // Configure the channel input
    inputCode = inputCode & 0b00000111;         // Bitwise AND to get just the bits we want and set the rest to zero
    config = (config & 0b11111000) | inputCode; // Bitwise AND to clear the input bits and Bitwise OR to set the new input bits

    // Write the new configuration
    ADS1299::WREG(reg,config); delay(1);

	isRunning = false;
};

/**
 * @brief Configures the connection of the positive pin of the given channel
 * to the bias drive of the ADS1299 device.
 * 
 * @param N The channel number to be configured.
 * @param connect A bool value indicating whether the positive pin should be
 * connected (true) or disconnected (false).
 */
void ADS1299Manager::configureChannelPositiveBias(int N, bool connect) {
	byte config;
        
    // Check the inputs
    if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
    
    // Disable any data collection
    ADS1299::SDATAC(); delay(1);

    // Read the previous configuration
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

	config = ADS1299::RREG(BIAS_SENSP); delay(1);

	// Configure the channel input
	if (connect) bitSet(config,N_zeroRef);
	else bitClear(config,N_zeroRef);

	// Write the new configuration
	ADS1299::WREG(BIAS_SENSP,config); delay(1);
}

/**
 * @brief Configures the connection of the negative pin of the given channel
 * to the bias drive of the ADS1299 device.
 * 
 * @param N The channel number to be configured.
 * @param connect A bool value indicating whether the negative pin should be
 * connected (true) or disconnected (false).
 */
void ADS1299Manager::configureChannelNegativeBias(int N, bool connect) {
	byte config;
		
	// Check the inputs
	if ((N < 1) || (N > NCHAN_PER_BOARD)) return;
	
	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	// Read the previous configuration
	int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

	config = ADS1299::RREG(BIAS_SENSN); delay(1);

	// Configure the channel input
	if (connect) bitSet(config,N_zeroRef);
	else bitClear(config,N_zeroRef);

	// Write the new configuration
	ADS1299::WREG(BIAS_SENSN,config); delay(1);
}

/**
 * @brief Determines if the bias drive of the ADS1299 device is powered or not.
 * 
 * @param connect A bool value indicating whether the bias drive should be
 * powered (true) or not (false).
 */
void ADS1299Manager::powerBiasDrive(bool connect) {
	byte config;
	
	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	config = ADS1299::RREG(CONFIG3); delay(1);

	// Configure the channel input
	if (connect) bitSet(config,2);
	else bitClear(config,2);

	// Write the new configuration
	ADS1299::WREG(CONFIG3,config); delay(1);
}

/**
 * @brief Configures the source of the positive input of the bias drive, BIASREF
 * of the ADS1299 device.
 * 
 * @param connect A bool value indicating whether BIASREF should be provided by
 * external reference (false) or fixed to (AVDD + AVSS)/2 provided internally
 * (true).
 */
void ADS1299Manager::configureBiasRefInt(bool connect) {
	byte config;
	
	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	config = ADS1299::RREG(CONFIG3); delay(1);

	// Configure the channel input
	if (connect) bitSet(config,3);
	else bitClear(config,3);

	// Write the new configuration
	ADS1299::WREG(CONFIG3,config); delay(1);
}

/**
 * @brief Configures the channel N to measure the bias.
 * 
 * @param N The channel number to be configured.
 */
void ADS1299Manager::setBiasMeas(int N){
	int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

	// First set channel to measure bias
	configureChannelInput(N, ADSINPUT_BIAS_MEAS);

	// Then set the bias measurement
	byte config = ADS1299::RREG(CONFIG3); delay(1);
	bitSet(config,4); // Set the BIAS_MEAS

	// Write the new configuration
	ADS1299::WREG(CONFIG3,config); delay(1);
}

/**
 * @brief Unsets the bias measurement of the ADS1299 device.
 * 
 */
void ADS1299Manager::unsetBiasMeas(){
	
	byte config = ADS1299::RREG(CONFIG3); delay(1);
	bitClear(config,4); // Set the BIAS_MEAS

	// Write the new configuration
	ADS1299::WREG(CONFIG3,config); delay(1);
}

/**
 * @brief Checks if the given channel of the ADS1299 device is active.
 * 
 * @param N The channel number to be checked.
 * @return bool value indicating whether the channel is active (true) or not
 * 
 * @note The channel number N is one-referenced (i.e., [1...N]), not [0...N-1].
 */
bool ADS1299Manager::isChannelActive(int N) {
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

    // Get the current configuration of the byte
    byte reg = CH1SET+(byte)N_zeroRef;
    byte config = ADS1299::RREG(reg); delay(1);

    // Read the left-most bit (bit 7) of the CHnSET register
    bool chanState = bitRead(config,7);

    return chanState;
};

/**
 * @brief Reads the gain of the given channel of the ADS1299 device.
 * 
 * @param N The channel number to be checked.
 * @return int value indicating the gain of the channel. If the channel is not
 * using the PGA, it returns -1. * 
 */
int ADS1299Manager::getChannelGain(int N) {
    int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

    // Get the current configuration of the byte
    byte reg = CH1SET+(byte)N_zeroRef;
    byte config = ADS1299::RREG(reg); delay(1);

    // Read the gain bits (bits [6:4]) of the CHnSET register
    config = (config & 0b01110000);

    switch (config) {
        case ADS_GAIN01:
            return 1;
        case ADS_GAIN02:
            return 2;
        case ADS_GAIN04:
            return 4;
        case ADS_GAIN06:
            return 6;
        case ADS_GAIN08:
            return 8;
        case ADS_GAIN12:
            return 12;
        case ADS_GAIN24:
            return 24;
        case ADS_GAIN00:
            return -1; // This does not use the PGA
    }
};

/**
 * @brief Configures the test signal of the ADS1299 device.
 * 
 * This function configures the test signal of the ADS1299 device by setting the
 * amplitude and frequency of the test signal in CONFIG2 register. To see the
 * test signal make sure to connect the test signal to the desired channel using
 * the configureChannelInput function.
 * 
 * @param amplitudeCode The amplitude code to be set. Available amplitude codes
 * have been defined as macros with the format ADSTESTSIG_AMP_XX, where XX is the
 * amplitude factor, that can be 1X or 2X.
 * @param freqCode The frequency code to be set. Available frequency codes have
 * been defined as macros with the format ADSTESTSIG_PULSE_XXXX, where XXXX is
 * the frequency of the test signal, that can be SLOW or FAST. Also, it is
 * possible to use the ADSTESTSIG_DCSIG macro to set the test signal as a DC
 * signal.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 * @note The amplitudeCode and freqCode parameters can be set to ADSTESTSIG_NOCHANGE
 * to keep the current configuration of the CONFIG2 register.
 */
void ADS1299Manager::configureInternalTestSignal(byte amplitudeCode, byte freqCode) {

	if (amplitudeCode == ADSTESTSIG_NOCHANGE) amplitudeCode = (ADS1299::RREG(CONFIG2) & (0b00000100));
	if (freqCode == ADSTESTSIG_NOCHANGE) freqCode = (ADS1299::RREG(CONFIG2) & (0b00000011));
	freqCode &= 0b00000011;  //only the last two bits should be used
	amplitudeCode &= 0b00000100;  //only this bit should be used
	byte message = 0b11010000 | freqCode | amplitudeCode;  //compose the code
	
	ADS1299::WREG(CONFIG2,message); delay(1);
};

/**
 * @brief Connects or disconnects the SRB1 pin to all negative inputs of the
 * channels in the ADS1299 device.
 * 
 * @param connect A bool value indicating whether the SRB1 pin should be
 * connected (true) or disconnected (false).
 */
void ADS1299Manager::connectSRB1(bool connect) {
	byte reg = 0b00000000;

	if (connect) bitSet(reg,5);

	ADS1299::WREG(MISC1, reg); delay(1);
};

/**
 * @brief Connects or disconnects the SRB2 pin to the given channel of the ADS1299
 * device.
 * 
 * @param N The channel number to be connected.
 * @param connect A bool value indicating whether the SRB2 pin should be
 * connected (true) or disconnected (false).
 */
void ADS1299Manager::connectSRB2(int N, bool connect) {
	int N_zeroRef = constrain(N-1,0,NCHAN_PER_BOARD-1);

    // Get the current configuration of the byte
    byte reg = CH1SET+(byte)N_zeroRef;
    byte config = ADS1299::RREG(reg); delay(1);

	// Set OR unset the SRB2 bit
	if (connect) bitSet(config,3);
	else bitClear(config,3);

	// Write the new configuration
	ADS1299::WREG(reg,config); delay(1);
};

/**
 * @brief Enters the Read Data Continuous mode of the ADS1299 device.
 * 
 * This function enters the Read Data Continuous mode of the ADS1299 device by
 * sending the RDATAC command to the ADS1299 device. This mode allows the device
 * to continuously send data to the Arduino when a START command is sent or when
 * the START pin is set to high.
*/
void ADS1299Manager::startDataContinuous(void)
{
	ADS1299::RDATAC(); delay(1);           // enter Read Data Continuous mode

	isRunning = true;
};

/**
 * @brief Exits the Read Data Continuous mode of the ADS1299 device.
 * 
 * This function exits the Read Data Continuous mode of the ADS1299 device by
 * sending the SDATAC command to the ADS1299 device. This mode allows the device
 * to stop sending data to the Arduino.
 */
void ADS1299Manager::stopDataContinuous(void)
{
	ADS1299::SDATAC(); delay(1);           // exit Read Data Continuous mode to communicate with ADS

	isRunning = false;
};
 
/**
 * @brief Starts the data acquisition of the ADS1299 device by entering the Read
 * Data Continuous mode and starting the data acquisition.
 * 
 * @note This function assumes that the ADS1299 device is already initialized
 * and connected.
 */
void ADS1299Manager::startStreaming(void)
{
    ADS1299::START();                      //start the data acquisition
}

/**
 * @brief Controls single-shot mode on the ADS1299 device.
 *
 * @param singleShotMode A bool value indicating whether to enter single-shot
 * mode (true) or continuous mode (false). 
 */
void ADS1299Manager::singleShotMode(bool enableSingleShotMode) {
	// Disable any data collection
	ADS1299::SDATAC(); delay(1);

	byte config = ADS1299::RREG(CONFIG4); delay(1);
	if (enableSingleShotMode) bitSet(config,3); // Set the single-shot mode
	else bitClear(config,3); // Set the continuous mode

	// Write the new configuration
	ADS1299::WREG(CONFIG4,config); delay(1);
}

/**
 * @brief Does a single read of the ADS1299 device using the RDATA command.
 * 
 */
void ADS1299Manager::singleRead(void){
	ADS1299::START(); delay(1);
	ADS1299::STOP(); delay(1);
	
	while(!ADS1299Manager::isDataAvailable()){
		// Await for the data to be available
	}

	ADS1299::RDATA();
	ADS1299Manager::printData();

}

/**
 * @brief Checks if the data is available to be read from the ADS1299 device by
 * reading the DRDY pin.
 * 
 * @return int value indicating whether the data is available (1) or not (0).
 */
int ADS1299Manager::isDataAvailable(void)
{
  return (!(digitalRead(PIN_DRDY)));
}
  
/**
 * @brief Stops the data acquisition of the ADS1299 device by stopping the data
 * acquisition and exiting the Read Data Continuous mode.
 * 
 */
void ADS1299Manager::stopStreaming(void)
{
    ADS1299::STOP(); delay(1);

	isRunning = false;
}

/**
 * @brief Reads N samples from the ADS1299 device and returns them via the
 * serial port.
 * 
 * @param N The number of samples to be read from the ADS1299 device.
 * 
 */
void ADS1299Manager::batchRead(int N)
{
	if (N < 1){
		Serial.println("Error: N must be greater than 0");
		return;
	}

	batchSize = N;

	// Read the samples from the ADS1299 device
	for (int i = 0; i < N; i++) {

		while(!ADS1299Manager::isDataAvailable()){
			// Await for the data to be available
		}
		ADS1299::updateChannelData(); // Read the data from the ADS1299 device

		for (int j = 0; j < ADS1299Manager::N_CHANS; j++) {
			batchData[i][j] = ADS1299Manager::channelData[j];
		}
	}

	ADS1299::SDATAC(); delay(1);
}

void ADS1299Manager::getBatch(){

	if (batchSize < 1){
		Serial.println("Error: batchSize must be greater than 0. Remember to call batchRead() first.");
		return;
	}

	for (int i = 0; i < batchSize; i++) {
		// Print the samples to the serial port
		for (int chan = 0; chan < ADS1299Manager::N_CHANS; chan++ )
		{
			Serial.print(batchData[i][chan]);
			Serial.print(", ");
		}
		Serial.println();
	}

	batchSize = 0; // Reset the batch size after reading
};
  
/**
 * @brief Prints the data of the first N channels of the ADS1299 device as text
 * to the Serial port.
 * 
 * @param N The number of channels to be printed.
 * @param sampleNumber If greater than zero, it will be printed at the start of
 * the line.
 * @param sendAuxValue If true, it will print the auxiliary value at the end of
 * the line.
 * @param auxValue The auxiliary value to be printed at the end of the line.
 */
void ADS1299Manager::printChannelDataAsText(int N, long int sampleNumber,
     bool sendAuxValue, long int auxValue)
{
	//check the inputs
	if ((N < 1) || (N > nChans)) return;
	
	//print the sample number, if not disabled
	if (sampleNumber > 0) {
		Serial.print(sampleNumber);
		Serial.print(", ");
	}

	//print each channel
	for (int chan = 0; chan < N; chan++ )
	{
		Serial.print(channelData[chan]);
		Serial.print(", ");
	}

	//print end of line
	Serial.println();
};

/**
 * @brief Prints the data of the first N channels of the ADS1299 device as
 * telemetry text to the Serial port.
 * 
 * @param N The number of channels to be printed.
 * @param sampleNumber If greater than zero, it will be printed at the start of
 * the line.
 * @param sendAuxValue If true, it will print the auxiliary value at the end of
 * the line.
 * @param auxValue The auxiliary value to be printed at the end of the line.
 */
void ADS1299Manager::printChannelDataAsTelemetry(int N, long int sampleNumber,
	 bool sendAuxValue, long int auxValue)
{
		//check the inputs
		if ((N < 1) || (N > nChans)) return;
	
		//print the sample number, if not disabled
		if (sampleNumber > 0) {
			Serial.print(">sample:");
			Serial.println(sampleNumber);
		}
	
		//print each channel
		for (int chan = 0; chan < N; chan++ )
		{
			Serial.print(">chan");
			Serial.print(chan+1);
			Serial.print(": ");
			Serial.println(channelData[chan]);
		}
};

// Auxiliar variable for binary communication
int32 val;
byte *val_ptr = (byte *)(&val);

/**
 * @brief Overloaded function to write the data of the first N channels of the
 * ADS1299 device as binary to the Serial port.
 * 
 * @param N The number of channels to be written.
 * @param sampleNumber If greater than zero, it will be written at the start of
 * the line.
 */
void ADS1299Manager::writeChannelDataAsBinary(int N, long sampleNumber){
	ADS1299Manager::writeChannelDataAsBinary(N,sampleNumber,false,0,false);
}

/**
 * @brief Overloaded function to write the data of the first N channels of the
 * ADS1299 device as binary to the Serial port.
 * 
 * @param N The number of channels to be written.
 * @param sampleNumber If greater than zero, it will be written at the start of
 * the line.
 * @param useSyntheticData If true, it will use synthetic data for the channels.
 */
void ADS1299Manager::writeChannelDataAsBinary(int N, long sampleNumber,bool useSyntheticData){
	ADS1299Manager::writeChannelDataAsBinary(N,sampleNumber,false,0,useSyntheticData);
}

/**
 * @brief Overloaded function to write the data of the first N channels of the
 * ADS1299 device as binary to the Serial port.
 * 
 * @param N The number of channels to be written.
 * @param sampleNumber If greater than zero, it will be written at the start of
 * the line.
 * @param auxValue The auxiliary value to be written at the end of the line.
 */
void ADS1299Manager::writeChannelDataAsBinary(int N, long sampleNumber,long int auxValue){
	ADS1299Manager::writeChannelDataAsBinary(N,sampleNumber,true,auxValue,false);
}

/**
 * @brief Overloaded function to write the data of the first N channels of the
 * ADS1299 device as binary to the Serial port.
 * 
 * @param N The number of channels to be written.
 * @param sampleNumber If greater than zero, it will be written at the start of
 * the line.
 * @param auxValue The auxiliary value to be written at the end of the line.
 * @param useSyntheticData If true, it will use synthetic data for the channels.
 */
void ADS1299Manager::writeChannelDataAsBinary(int N, long sampleNumber,long int auxValue, bool useSyntheticData){
	ADS1299Manager::writeChannelDataAsBinary(N,sampleNumber,true,auxValue,useSyntheticData);
}

/**
 * @brief Overloaded function to write the data of the first N channels of the
 * ADS1299 device as binary to the Serial port.
 * 
 * @param N The number of channels to be written.
 * @param sampleNumber If greater than zero, it will be written at the start of
 * the line.
 * @param sendAuxValue If true, it will write the auxiliary value at the end of
 * the line.
 * @param auxValue The auxiliary value to be written at the end of the line.
 * @param useSyntheticData If true, it will use synthetic data for the channels.
 * 
 * @note This version has the implementation of the binary communication protocol.
 */
void ADS1299Manager::writeChannelDataAsBinary(int N, long sampleNumber,bool sendAuxValue, 
	long int auxValue, bool useSyntheticData)
{
	//check the inputs
	if ((N < 1) || (N > nChans)) return;
	
	// Write start byte
	Serial.write( (byte) PCKT_START);
	
	//write the length of the payload
	//byte byte_val = (1+8)*4;
	byte payloadBytes = (byte)((1+N)*4);    //length of data payload, bytes
	if (sendAuxValue) payloadBytes+= (byte)4;  //add four more bytes for the aux value
	Serial.write(payloadBytes);  //write the payload length

	//write the sample number, if not disabled
	if (sampleNumber > 0) {
		val = sampleNumber;
		Serial.write(val_ptr,4); //4 bytes long
	}

	//write each channel
	for (int chan = 0; chan < N; chan++ )
	{
		//get this channel's data
		if (useSyntheticData) {
			val = makeSyntheticSample(sampleNumber,chan);
			//val = sampleNumber;
		} else {
			//get the real EEG data for this channel
			val = channelData[chan];
		}
		Serial.write(val_ptr,4); //4 bytes long
	}
	
	// Write the AUX value
	if (sendAuxValue) {
		val = auxValue;
		Serial.write(val_ptr,4); //4 bytes long
	}
	
	// Write footer
	Serial.write((byte)PCKT_END);
	
	// force everything out
	//Serial.flush();	
};

//write channel data using binary format of ModularEEG so that it can be used by BrainBay (P2 protocol)
//this only sends 6 channels of data, per the P2 protocol
//http://www.shifz.org/brainbay/manuals/brainbay_developer_manual.pdf
#define max_int16 (32767)
#define min_int16 (-32767)
void ADS1299Manager::writeChannelDataAsOpenEEG_P2(long sampleNumber) {
	ADS1299Manager::writeChannelDataAsOpenEEG_P2(sampleNumber,false);
}
void ADS1299Manager::writeChannelDataAsOpenEEG_P2(long sampleNumber,bool useSyntheticData) {
	static int count = -1;
	byte sync0 = 0xA5;
	byte sync1 = 0x5A;
	byte version = 2;
	
	Serial.write(sync0);
	Serial.write(sync1);
	Serial.write(version);
	byte foo = (byte)sampleNumber;
	if (foo == sync0) foo--;
	Serial.write(foo);
	
	long val32; //32-bit
	int val_i16;  //16-bit
	unsigned int val_u16;  //16-bit
	byte *val16_ptr = (byte *)(&val_u16);  //points to the memory for the variable above
	for (int chan = 0; chan < 6; chan++ )
	{
		//get this channel's data
		if (useSyntheticData) {
			//generate XX uV pk-pk signal
			//long time_samp_255 = (long)((sampleNumber) & (0x000000FF));  //make an 8-bit ramp waveform
			//time_samp_255 = (long)((time_samp_255*(long)(chan+1)) & (0x000000FF)); //each channel is faster than the previous
			//time_samp_255 += 256L*2L;  //make zero mean...empirically tuned via BrainBay visualization
			//val32 = (synthetic_amplitude_counts * time_samp_255) / 255L; //scaled zero-mean ramp 
			val32 = makeSyntheticSample(sampleNumber,chan) + 127L + 256L*2L;  //make zero mean...empirically tuned via BrainBay visualization
		} else {
			//get the real EEG data for this channel
			val32 = channelData[chan];
		}			
						
		//prepare the value for transmission
		val32 = val32 / (32);  //shrink to fit within a 16-bit number
		val32 = constrain(val32,min_int16,max_int16);  //constrain to fit in 16 bits
		val_u16 = (unsigned int) (val32 & (0x0000FFFF));  //truncate and cast
		if (val_u16 > 1023) val_u16 = 1023;
	
		//Serial.write(val16_ptr,2); //low byte than high byte on Arduino
		//Serial.write((byte)((val_u16 >> 8) & 0x00FF)); //high byte
		//Serial.write((byte)(val_u16 & 0x00FF)); //low byte
		foo = (byte)((val_u16 >> 8) & 0x00FF); //high byte
		if (foo == sync0) foo--;
		Serial.write(foo);
		foo = (byte)(val_u16 & 0x00FF); //high byte
		if (foo == sync0) foo--;
		Serial.write(foo);


		
	}
	//byte switches = 0b00000000;  //the last thing required by the P2 data protocol
	byte switches = 0x07;
	count++; if (count >= 18) count=0;
	if (count >= 9) {
		switches = 0x0F;
	}	
	Serial.write(switches);
}

#define synthetic_amplitude_counts (8950L)   //counts peak-to-peak...should be 200 uV pk-pk  2.0*(100e-6 / (4.5 / 24 / 2^24))
long int ADS1299Manager::makeSyntheticSample(long sampleNumber,int chan) {
	//generate XX uV pk-pk signal
	long time_samp_255 = (long)((sampleNumber) & (0x000000FF));  //make an 8-bit ramp waveform
	time_samp_255 = (long)((time_samp_255*(long)(chan+1)) & (0x000000FF)); //each channel is faster than the previous
	//time_samp_255 += 256L*2L;  //make zero mean...empirically tuned via BrainBay visualization
	time_samp_255 -= 127L;
	return (synthetic_amplitude_counts * time_samp_255) / 255L; //scaled zero-mean ramp 
};

//print out the state of all the control registers
void ADS1299Manager::printAllRegisters(void)   
{
	bool prevVerboseState = verbose;
	
        verbose = true;
        ADS1299::RREGS(0x00,0x10);     // write the first registers
        delay(100);  //stall to let all that data get read by the PC
        ADS1299::RREGS(0x11,0x17-0x11);     // write the rest
        verbose = prevVerboseState;
};

/**
 * @brief Overloaded version of printData that just prints the data of all
 * available channels of the ADS1299s device in the selected format to the
 * Serial port.
 * 
 */
void ADS1299Manager::printData() {
	ADS1299Manager::printData(nChans, -1, false, 0,false);
}

void ADS1299Manager::printData(long sampleNumber) {
	ADS1299Manager::printData(nChans, sampleNumber, false, 0,false);
}

void ADS1299Manager::printData(long sampleNumber, bool sendAuxValue, long int auxValue) {
	ADS1299Manager::printData(nChans, sampleNumber,sendAuxValue,auxValue,false);
}

/**
 * @brief Prints the data of the first N channels of the ADS1299 device in the
 * selected format to the Serial port.
 * 
 * @param outputType The output format to be used. It can be OUTPUT_TYPE_TEXT,
 * OUTPUT_TYPE_BINARY, or OUTPUT_TYPE_OPENEEG.
 * @param N The number of channels to be printed.
 * @param sampleNumber If greater than zero, it will be printed at the start of
 * the line.
 * @param sendAuxValue If true, it will print the auxiliary value at the end of
 * the line.
 * @param auxValue The auxiliary value to be printed at the end of the line.
 * @param useSyntheticData If true, it will use synthetic data for the channels.
 * 
 * @note Text format does not supports synthetic data.
 */
void ADS1299Manager::printData(int N, long sampleNumber, bool sendAuxValue, long int auxValue, bool useSyntheticData) {
	switch (outputMode) {
		case OUTPUT_TYPE_TEXT:
			ADS1299Manager::printChannelDataAsText(N,sampleNumber, sendAuxValue, auxValue);
			break;
		case OUTPUT_TYPE_BINARY:
			ADS1299Manager::writeChannelDataAsBinary(N,sampleNumber,sendAuxValue,auxValue,useSyntheticData);
			break;
		case OUTPUT_TYPE_OPENEEG:
			ADS1299Manager::writeChannelDataAsOpenEEG_P2(sampleNumber,useSyntheticData);
			break;
		case OUTPUT_TYPE_TELEMETRY:
			ADS1299Manager::printChannelDataAsTelemetry(N,sampleNumber,sendAuxValue,auxValue);
			break;
	}
};

/**
 * @brief Prints the help message for the ADS1299 driver.
 * 
 * This function prints the help message for the ADS1299 driver. It lists all
 * the available commands and their usage. * 
 */
void ADS1299Manager::printHelp(void) {
	Serial.println("ADS1299 driver help. Available commands:");
	Serial.print("\t"); Serial.print(ENABLE_COMMAND); Serial.print("n - Enable channel n.\n\t\tFor example, '"); Serial.print(ENABLE_COMMAND); Serial.println("1' enables channel 1.");

	Serial.print("\t"); Serial.print(DISABLE_COMMAND); Serial.print("n - Disable channel n.\n\t\tFor example, '"); Serial.print(DISABLE_COMMAND); Serial.println("1' disables channel 1.");

	Serial.print("\t"); Serial.print(CLOCK_OUTPUT_FLAG); Serial.print("X - Enable the clock output.\n\t\tFor example, '"); Serial.print(CLOCK_OUTPUT_FLAG); Serial.print(CLOCK_OUTPUT_ENABLE); Serial.println("' enables the clock output.");
	Serial.print("\t\tAvailable clock output codes: "); Serial.print(CLOCK_OUTPUT_ENABLE); Serial.print(" (enable), and "); Serial.print(CLOCK_OUTPUT_DISABLE); Serial.println(" (disable).");

	Serial.print("\t"); Serial.print(DATA_RATE_FLAG); Serial.print("X - Set the data rate to X.\n\t\tFor example, '"); Serial.print(DATA_RATE_FLAG); Serial.print(DATA_RATE_250); Serial.println("' sets the data rate to 250 SPS.");
	Serial.print("\t\tAvailable data rates codes: "); Serial.print(DATA_RATE_250); Serial.print(" (250 SPS), "); Serial.print(DATA_RATE_500); Serial.print(" (500 SPS), "); Serial.print(DATA_RATE_1K); Serial.print(" (1000 SPS), "); Serial.print(DATA_RATE_2K); Serial.print(" (2000 SPS), "); Serial.print(DATA_RATE_4K); Serial.print(" (4000 SPS), "); Serial.print(DATA_RATE_8K); Serial.print(" (8000 SPS), and "); Serial.print(DATA_RATE_16K); Serial.println(" (16000 SPS)");

	Serial.print("\t"); Serial.print(GAIN_FLAG); Serial.print("nX - Set the gain of channel n to X.\n\t\tFor example, '"); Serial.print(GAIN_FLAG); Serial.println("14' sets the gain of channel 1 to 4.");
	Serial.println("\t\tAvailable gains: 1, 2, 4, 6, 8, 12, 24, 0 (PGA bypassed).");

	Serial.print("\t"); Serial.print(INPUT_FLAG); Serial.print("nX - Set the input configuration of channel n to X.\n\t\tFor example, '"); Serial.print(INPUT_FLAG); Serial.println("1n' sets the input configuration of channel 1 to normal.");
	Serial.print("\t\tAvailable inputs: "); Serial.print(NORMAL_INPUT); Serial.print(" (normal), "); Serial.print(SHORTED_INPUT); Serial.print(" (shorted), "); Serial.print(TEST_SIGNAL_INPUT); Serial.println(" (test signal).");

	Serial.print("\t"); Serial.print(BIAS_FLAG); Serial.print("["); Serial.print(BIAS_POWER_FLAG); Serial.print("[1|0]|"); Serial.print(BIASREF_FLAG); Serial.print("["); Serial.print(BIASREF_INTERNAL); Serial.print("|"); Serial.print(BIASREF_EXTERNAL); Serial.print("]"); Serial.print("|"); Serial.print("["); Serial.print(BIAS_PCHAN_FLAG); Serial.print("|"); Serial.print(BIAS_NCHAN_FLAG); Serial.print("]");Serial.print("["); Serial.print(BIAS_CHAN_ENABLE); Serial.print("|"); Serial.print(BIAS_CHAN_DISABLE); Serial.print("][channel]|");Serial.print(BIAS_MEAS_FLAG); Serial.print("["); Serial.print(BIAS_MEAS_ENABLE); Serial.print("|"); Serial.print(BIAS_MEAS_DISABLE); Serial.println("][channel]] - Configure the bias drive.");
	Serial.print("\t\tSubcommand "); Serial.print(BIAS_POWER_FLAG); Serial.println(" enables (1) or disables (0) the bias drive.");
	Serial.print("\t\tSubcommand "); Serial.print(BIASREF_FLAG); Serial.println(" sets the source of the bias reference to internal (i) or external (e).");
	Serial.print("\t\tSubcommand "); Serial.print(BIAS_PCHAN_FLAG); Serial.println(" connects (e) or disconnects (d) the positive input of channel (channel) to the bias drive.");
	Serial.print("\t\tSubcommand "); Serial.print(BIAS_NCHAN_FLAG); Serial.println(" connects (e) or disconnects (d) the negative input of channel (channel) to the bias drive.");
	Serial.print("\t\tSubcommand "); Serial.print(BIAS_MEAS_FLAG); Serial.println(" connects (e) the channel (channel) bias measurement or disconnects (d) it.");

	Serial.print("\t"); Serial.print(REFERENCE_FLAG); Serial.print("XYN - Set/unset (Y) reference X in channel N.\n\t\tFor example, '"); Serial.print(REFERENCE_FLAG); Serial.println("11' connects all negative inputs to SRB1.");
	Serial.println("\t\tAvailable references (X): SRB1 (1) and SRB2 (2). For SRB1, channel is ignored.");
	Serial.println("\t\tAvailable connections (Y): Connect (1), Disconnect (0).");

	Serial.print("\t"); Serial.print(TEST_SIGNAL_FLAG); Serial.print("AF - Set the test signal to amplitude A and frequency F.\n\t\tFor example, '"); Serial.print(TEST_SIGNAL_FLAG); Serial.println("1S' sets the test signal to 1X amplitude and slow frequency.");
	Serial.print("\t\tAvailable amplitudes: "); Serial.print(AMP_1X); Serial.print(" (1X), "); Serial.print(AMP_2X); Serial.println(" (2X).");
	Serial.print("\t\tAvailable frequencies: "); Serial.print(PULSE_SLOW); Serial.print(" (slow), "); Serial.print(PULSE_FAST); Serial.print(" (fast), "); Serial.print(DC_SIGNAL); Serial.println(" (DC signal).");

	Serial.print("\t"); Serial.print(RDATAC_MODE); Serial.println(" - Enter the Read Data Continuous mode.");
	Serial.print("\t"); Serial.print(SDATAC_MODE); Serial.println(" - Exit the Read Data Continuous mode.");
	
	Serial.print("\t"); Serial.print(START_STREAMING); Serial.println(" - Start streaming data.");
	Serial.print("\t"); Serial.print(STOP_STREAMING); Serial.println(" - Stop streaming data.");

	Serial.print("\t"); Serial.print(SINGLE_SHOT_MODE); Serial.print("X - Enable (1) or disable (0) the single-shot mode.\n\t\tFor example, '"); Serial.print(SINGLE_SHOT_MODE); Serial.println("1' enables the single-shot mode.");

	Serial.print("\t"); Serial.print(SINGLE_READ); Serial.println(" - Do a single conversion.");

	Serial.print("\t"); Serial.print(BATCH_READ); Serial.print("N - Read N samples from the ADS1299 device.\n\t\tFor example, '"); Serial.print(BATCH_READ); Serial.println("10' reads 10 samples from the ADS1299 device and then prints them to the Serial port.");

	Serial.print("\t"); Serial.print(GET_BATCH); Serial.println(" - Get the batch of samples read with the BATCH_READ command.");

	Serial.print("\t"); Serial.print(PRINT_REGISTERS); Serial.println(" - Print the state of all the control registers.");

	Serial.print("\t"); Serial.print(CONFIGURE_OUTPUT_FLAG); Serial.print("n - Configure the output format to n.\n\t\tFor example, '"); Serial.print(CONFIGURE_OUTPUT_FLAG); Serial.println("0' sets the output format to text mode.");
	Serial.print("\t\tAvailable output formats: "); Serial.print(OUTPUT_TYPE_TEXT); Serial.print(" (text), "); Serial.print(OUTPUT_TYPE_BINARY); Serial.print(" (binary), "); Serial.print(OUTPUT_TYPE_OPENEEG); Serial.print(" (OpenEEG)."); Serial.print(OUTPUT_TYPE_TELEMETRY); Serial.println(" (telemetry).");

	Serial.print("\t"); Serial.print(VERBOSE_FLAG); Serial.print("X - Enable (1) or disable (0) the verbose mode.\n\t\tFor example, '"); Serial.print(VERBOSE_FLAG); Serial.println("1' enables the verbose mode.");

	Serial.print("\t"); Serial.println("h - Print this help.");
};

/**
 * @brief This function processes the input commands from the Serial port.
 * 
 * This function processes the input commands from the Serial port. It is used
 * to change the configuration of the ADS1299 device. First, it stops the data
 * acquisition, then it acts according to the command received. Finally, it
 * restarts the data acquisition.
 * 
 * @param command The command to be processed. Check the Commands.h file for
 * more information about the available commands.
 */
void ADS1299Manager::processInputCommands(String command) {

	bool wasRunning = isRunning;
	bool invalidCommand = false;

	// Stop the data acquisition
	stopStreaming();

	// Process the command. Start by checking the first character of the command
	switch (command.charAt(0))
	{

		case ENABLE_COMMAND:{ // Enable a channel
			int channel = command.substring(1).toInt();
			if (channel < 1 || channel > NCHAN_PER_BOARD) {
				if (managerVerbose) {
					Serial.print("Invalid channel: ");
					Serial.println(channel);
				}
			} else {
				enableChannel(channel);
				if (managerVerbose) {
					Serial.print("Channel ");
					Serial.print(channel);
					Serial.println(" enabled.");
				}
			}
		}
		break;
		
		case DISABLE_COMMAND:{ // Disable a channel
			int channel = command.substring(1).toInt();
			if (channel < 1 || channel > NCHAN_PER_BOARD) {
				if (managerVerbose) {
					Serial.print("Invalid channel: ");
					Serial.println(channel);
				}
			} else {
				deactivateChannel(channel);
				if (managerVerbose) {
					Serial.print("Channel ");
					Serial.print(channel);
					Serial.println(" disabled.");
				}
			}
		}
		break;
		
		case GAIN_FLAG:{ // Set the gain of a channel
			int channel = command.substring(1,2).toInt();
			int gain = command.substring(2).toInt();
			
			switch (gain)
			{
				case GAIN_01:
					configureChannelGain(channel,ADS_GAIN01);
				break;

				case GAIN_02:
					configureChannelGain(channel,ADS_GAIN02);
				break;
				
				case GAIN_04:
					configureChannelGain(channel,ADS_GAIN04);
				break;
				
				case GAIN_06:
					configureChannelGain(channel,ADS_GAIN06);
				break;
				
				case GAIN_08:
					configureChannelGain(channel,ADS_GAIN08);
				break;
				
				case GAIN_12:
					configureChannelGain(channel,ADS_GAIN12);
				break;
				
				case GAIN_24:
					configureChannelGain(channel,ADS_GAIN24);
				break;
				
				case GAIN_00:
					configureChannelGain(channel,ADS_GAIN00);
				break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid gain: ");
						Serial.println(gain);
					}
					invalidCommand = true;
				break;
			}

			if (managerVerbose && !invalidCommand) {
				Serial.print("Channel ");
				Serial.print(channel);
				Serial.print(" gain set to ");
				Serial.println(gain);
			}
		}
		break;
		
		case INPUT_FLAG:{ // Set the input configuration of a channel
			int channel = command.substring(1,2).toInt();
			char input = command.charAt(2);
			
			switch (input)
			{
				case NORMAL_INPUT:
					configureChannelInput(channel,ADSINPUT_NORMAL);
				break;

				case SHORTED_INPUT:
					configureChannelInput(channel,ADSINPUT_SHORTED);
				break;
				
				case TEST_SIGNAL_INPUT:
					configureChannelInput(channel,ADSINPUT_TESTSIG);
				break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid input: ");
						Serial.println(input);
					}
					invalidCommand = true;
				break;
				
			}

			if (managerVerbose && !invalidCommand) {
				Serial.print("Channel ");
				Serial.print(channel);
				Serial.print(" input set to ");
				Serial.println(input);
			}
		}
		break;

		case CLOCK_OUTPUT_FLAG:{ // Set the clock output configuration
			char clock = command.charAt(1);
			
			switch (clock)
			{
				case CLOCK_OUTPUT_ENABLE:
					setClockOutput(true);
					if (managerVerbose) {
						Serial.println("Clock output enabled.");
					}
				break;

				case CLOCK_OUTPUT_DISABLE:
					setClockOutput(false);
					if (managerVerbose) {
						Serial.println("Clock output disabled.");
					}
				break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid clock output: ");
						Serial.println(clock);
					}
				break;
			}
		}
		break;

		case DATA_RATE_FLAG:{
			char ratecode = command.charAt(1);
			int rate = 0;
			
			switch (ratecode)
			{
				case DATA_RATE_250:
					configureDataRate(ADS_DR_250);
					rate = 250;
				break;

				case DATA_RATE_500:
					configureDataRate(ADS_DR_500);
					rate = 500;
				break;
				
				case DATA_RATE_1K:
					configureDataRate(ADS_DR_1K);
					rate = 1000;
				break;
				
				case DATA_RATE_2K:
					configureDataRate(ADS_DR_2K);
					rate = 2000;
				break;
				
				case DATA_RATE_4K:
					configureDataRate(ADS_DR_4K);
					rate = 4000;
				break;
				
				case DATA_RATE_8K:
					configureDataRate(ADS_DR_8K);
					rate = 8000;
				break;
				
				case DATA_RATE_16K:
					configureDataRate(ADS_DR_16K);
					rate = 16000;
				break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid data rate: ");
						Serial.println(ratecode);
					}
					invalidCommand = true;
				break;
			}

			if (managerVerbose && !invalidCommand) {
				Serial.print("Data rate set to ");
				Serial.print(rate);
				Serial.println(" SPS.");
			}
		}
		break;

		case BIAS_FLAG:{
			char subflag = command.charAt(1);
			
			switch (subflag)
			{
				case BIAS_POWER_FLAG:{
					bool connect = command.substring(2).toInt();
					powerBiasDrive(connect);
					if (managerVerbose) {
						if (connect) {
							Serial.println("Bias drive powered.");
						} else {
							Serial.println("Bias drive unpowered.");
						}
					}	
				}
				break;

				case BIASREF_FLAG:{
					char mode = command.charAt(2);
					
					switch (mode)
					{
					case BIASREF_INTERNAL:
						configureBiasRefInt(true);

						if (managerVerbose) {
							Serial.println("Internal bias reference enabled.");
						}
					break;

					case BIASREF_EXTERNAL:
						configureBiasRefInt(false);

						if (managerVerbose) {
							Serial.println("External bias reference enabled.");
						}
					break;

					default:
						if (managerVerbose) {
							Serial.print("Invalid bias reference: ");
							Serial.println(mode);
						}
					break;
					}
				}
				break;

				case BIAS_PCHAN_FLAG:{
					char connect = command.charAt(2);
					int channel = command.substring(3).toInt();
					
					if (connect == BIAS_CHAN_ENABLE) {
						configureChannelPositiveBias(channel,true);
						if (managerVerbose) {
							Serial.print("Channel ");
							Serial.print(channel);
							Serial.println(" positive bias connected.");
						}
					} else {
						configureChannelPositiveBias(channel,false);
						if (managerVerbose) {
							Serial.print("Channel ");
							Serial.print(channel);
							Serial.println(" positive bias disconnected.");
						}
					}
				}
				break;

				case BIAS_NCHAN_FLAG:{
					char connect = command.charAt(2);
					int channel = command.substring(3).toInt();
					
					if (connect == BIAS_CHAN_ENABLE) {
						configureChannelNegativeBias(channel,true);
						if (managerVerbose) {
							Serial.print("Channel ");
							Serial.print(channel);
							Serial.println(" negative bias connected.");
						}
					} else {
						configureChannelNegativeBias(channel,false);
						if (managerVerbose) {
							Serial.print("Channel ");
							Serial.print(channel);
							Serial.println(" negative bias disconnected.");
						}
					}
				}
				break;

				case BIAS_MEAS_FLAG:{
					char connect = command.charAt(2);
					int channel = command.substring(3).toInt();

					if (connect == BIAS_CHAN_ENABLE) {
						setBiasMeas(channel);
						if (managerVerbose) {
							Serial.print("Channel ");
							Serial.print(channel);
							Serial.println(" connected to bias measurement.");
						}
					} else {
						unsetBiasMeas();
						if (managerVerbose) {
							Serial.println("Bias measurement disconnected.");
						}
					}
				}
				break;
			}
		}
		break;

		case REFERENCE_FLAG:{ // Set the reference configuration
			char reference = command.charAt(1);
			bool connect = command.substring(2, 3).toInt();
			int channel = command.substring(3).toInt();
			
			switch (reference)
			{
				case SRB1_FLAG:
					connectSRB1(connect);
					break;

				case SRB2_FLAG:
					connectSRB2(channel,connect);
					break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid reference: ");
						Serial.println(reference);
					}
					invalidCommand = true;
				break;
			}

			if (managerVerbose && !invalidCommand) {
				Serial.print("Reference SRB");
				Serial.print(reference);
				if (connect) {
					Serial.print(" connected");
				} else {
					Serial.print(" disconnected");
				}

				if (reference == SRB2_FLAG) {
					Serial.print(" to channel ");
					Serial.print(channel);
				}
				Serial.println(".");
			}
		}
		break;
		
		case TEST_SIGNAL_FLAG:{ // Set the test signal
			char amplitude = command.charAt(1);
			char frequency = command.charAt(2);
			
			byte amplitudeCode = 0;
			byte freqCode = 0;

			switch (amplitude)
			{
				case AMP_1X:
					amplitudeCode = ADSTESTSIG_AMP_1X;
				break;
				
				case AMP_2X:
					amplitudeCode = ADSTESTSIG_AMP_2X;
				break;

				default:
					if (managerVerbose) {
						Serial.print("Invalid amplitude: ");
						Serial.println(amplitude);
					}
					invalidCommand = true;
				break;
			
			}

			switch (frequency)
			{
				case PULSE_SLOW:
					freqCode = ADSTESTSIG_PULSE_SLOW;
				break;
				
				case PULSE_FAST:
					freqCode = ADSTESTSIG_PULSE_FAST;
				break;
				
				case DC_SIGNAL:
					freqCode = ADSTESTSIG_DCSIG;
				break;
				
				default:
					if (managerVerbose) {
						Serial.print("Invalid frequency: ");
						Serial.println(frequency);
					}
					invalidCommand = true;
				break;
			
			}

			if (!invalidCommand) {
				configureInternalTestSignal(amplitudeCode,freqCode);
			
				if (managerVerbose) {
					Serial.print("Test signal set to ");
					Serial.print(amplitude);
					Serial.print(" amplitude and ");
					Serial.println(frequency);

				}
			}
		}
		break;

		case RDATAC_MODE: // Enter Read Data Continuous mode
			startDataContinuous();
			if (managerVerbose) {
				Serial.println("Read Data Continuous mode entered.");
			}
		break;

		case SDATAC_MODE: // Exit Read Data Continuous mode
			stopDataContinuous();

			wasRunning = false; // Don't restart the streaming

			if (managerVerbose) {
				Serial.println("Read Data Continuous mode exited.");
			}
		break;

		case START_STREAMING: // Start streaming
			startStreaming();
		break;

		case STOP_STREAMING: // Stop streaming
			stopStreaming();

			if (managerVerbose) {
				Serial.println("Streaming stopped.");
			}
		break;

		case SINGLE_SHOT_MODE: { // Configure single-shot mode
			bool enableSingleShot = command.charAt(1) == '1';
			singleShotMode(enableSingleShot);
			if (managerVerbose) {
				if (enableSingleShot) {
					Serial.println("Single-shot mode enabled.");
				} else {
					Serial.println("Single-shot mode disabled.");
				}
			}
		}
		break;

		case SINGLE_READ:
			singleRead();
		break;

		case BATCH_READ: { // Read N samples
			int nSamples = command.substring(1).toInt();
			batchRead(nSamples);
		}
		break;

		case GET_BATCH: {
			getBatch();
		}
		break;

		case PRINT_REGISTERS: // Print registers
			printAllRegisters();
			break;

		case CONFIGURE_OUTPUT_FLAG:{
			// Configure the output format
			outputMode = command.substring(1).toInt();
			if (managerVerbose) {
				Serial.print("Output format set to ");
				Serial.println(outputMode);	
			}
		}
		break;
	
		case VERBOSE_FLAG:
			managerVerbose = command.substring(1).toInt();
			if (managerVerbose) {
				Serial.println("Verbose mode enabled.");
			} else {
				Serial.println("Verbose mode disabled.");
			}
		break;

		case 'h': // Print help
			printHelp();
		break;
		
		default:
			Serial.println("Command not recognized. Use the 'h' command to see the available commands.");
		break;
	}

	if (wasRunning) {
		startStreaming();
	}
};