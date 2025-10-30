# EEG Emulation and Acquisition

This repository contains the design files, code, and documentation for an EEG
emulation and acquisition system. The system is designed to emulate an EEG headset
using a custom PCB that demultiplexes signals from a National Instruments
USB-6212 Data Acquisition (DAQ) device, and to acquire EEG signals using an ADS1299
Analog-to-Digital Converter (ADC) connected to an Arduino Nano 33 BLE Sense.

## Repository Structure

The repository is organized into the following main directories and files:

- `docs/`: Documentation files for the project
    - `ads1299_driver/`: Python API reference for the ADS1299 driver
    - `assets/`: Images and diagrams used in documentation
    - `datasheets/`: Component datasheets and manuals
  
- `eeg_acquisition/`: EEG signal acquisition system components
    - `ADS1299driver.py`: Python wrapper for the ADS1299 driver
    - `design_files/`: KiCAD PCB and schematic files for the acquisition board
    - `micro/`: Arduino firmware for the ADS1299 driver
        - `src/`: C++ source code for the driver implementation

- `eeg_emulation/`: EEG signal emulation system components
    - `daq.py`: Python script for controlling the NI USB-6212 DAQ
    - `design_files/`: KiCAD PCB and schematic files for the demultiplexer board

- `test.py`: Example script demonstrating the integration between the EEG emulation and acquisition systems, including configuration of the ADS1299, signal generation, data acquisition, and visualization


## Requirements

### Software

- [Python](https://www.python.org/) 3.13 or later (for running the code and documentation)
    - Package manager, [`uv`](https://docs.astral.sh/uv/)
- [KiCAD](https://www.kicad.org/) 9 or later (for PCB design files)
- [PlatformIO](https://platformio.org/) (for Arduino development)

### Commercial hardware

- National Instruments NI USB-6212 Data Acquisition device

## Installation

- Clone the repository:

    ```bash
    git clone https://github.com/eneriz-daniel/bci-emulated-eeg-tflite.git
    ```

- Install the required packages:

    ```bash
    uv sync --all-groups
    ```

## Software commands

- To serve the documentation locally:

    ```bash
    uv run mkdocs serve
    ```

- To check linting and formatting:

    ```bash
    uv run ruff check
    ```
