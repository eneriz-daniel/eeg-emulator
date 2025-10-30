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


#include <ADS1299Manager.h>

ADS1299Manager ADSManager;

void setup() {

  ADSManager.initialize(false);  // Initialize the ADS1299 board. The parameter
                                 // is true for daisy-chain configuration

  Serial.begin(115200);          // Initialize Serial port

  Serial.println("ADS1299Manager driver");
  Serial.print("Configured for "); Serial.print(ADSManager.nChans); Serial.println(" channels");
  Serial.println("Using default settings for the ADS1299");

  ADSManager.printHelp(); // Print the help message
}

void loop() {
  // Catch any incoming commands
  if(Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    ADSManager.processInputCommands(command);
  }

  // if(ADSManager.isDataAvailable()){

  //   ADSManager.updateChannelData();

  //   ADSManager.printData();
  // }

  delayMicroseconds(1);
            
}