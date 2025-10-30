
## The ADC

We have selected the [ADS1299](datasheets/ads1299.pdf) as the Anaglog-to-Digital
Converter (ADC) for our EEG acquisition system. The ADS1299 is a low-power,
24-bit, 8-channel ADC designed specifically for biopotential measurements,
making it an ideal choice for EEG applications. It features a high input impedance,
low noise, and a built-in programmable gain amplifier (PGA), which allows for
accurate signal acquisition from the scalp electrodes.

The ADS1299 can be connected to a microcontroller using the SPI interface,
which allows for fast data transfer and control of the ADC settings. We have created
an schematic design on a Kicad project to document how we have connected the
ADS1299 to an Arduino Nano 33 BLE Sense. The schematic includes the necessary
connections for power supply, ground, and the SPI interface. These schematic files
are available at `eeg_acquisition/design_files`.

## The driver

We have extended an existing driver for the ADS1299 to provide an interface through
serial communication. The driver is written in C++ and it includes functions for
initializing the ADC, configuring the settings, and reading the data from the ADC.
This driver has been tested with the Arduino Nano 33 BLE Sense and it is compatible with
the Arduino IDE and PlatformIO. The driver is available at `eeg_acquisition/micro`, and it is
composed of two main files:

- `ADS1299.cpp` contains the low-level implementation of the driver, including the
SPI commands to configure the ADC and read the data. The driver uses the Arduino SPI library
to communicate with the ADS1299.

- `ADS1299Manager.cpp` contains the high-level implementation of the driver, including
the functions to initialize the ADC, configure the settings, and read the data.
It also provides the functionality to interface with serial communication, allowing
for easy integration with other systems.

Additionally, we have created a Python wrapper for the driver, available at
`eeg_acquisition/ADS1299driver.py`. This wrapper is able to communicate with the
Arduino board through serial communication, allowing for easy integration with
Python applications. The wrapper includes functions for configuring the ADC,
reading the data and even live plotting the acquired signals, as shown below.
A complete reference for the wrapper is available [here](ads1299_driver/python.md).
