//
//
// Commands.h
//
// This header file contains the definitions of the commands that can be sent to
// the microcontroller connected to the ADS1299 to change its configuration.
// Each command is a defined string

#ifndef _Commands_h
#define _Commands_h

// Channel identifiers
#define CHANNEL_1 1
#define CHANNEL_2 2
#define CHANNEL_3 3
#define CHANNEL_4 4
#define CHANNEL_5 5
#define CHANNEL_6 6
#define CHANNEL_7 7
#define CHANNEL_8 8

// Enable and disable channels: command = "[e|d][channel]"
// Example: "e1" enables channel 1
#define ENABLE_COMMAND 'e'
#define DISABLE_COMMAND 'd'

// Gain settings: command = "g[channel][gain]"
// Example: "g124" sets the gain of channel 1 to 24
#define GAIN_FLAG 'g'

#define GAIN_00 0
#define GAIN_01 1
#define GAIN_02 2
#define GAIN_04 4
#define GAIN_06 6
#define GAIN_08 8
#define GAIN_12 12
#define GAIN_24 24

// Clock output settings: command = "c[0|1]"
// Example: "c1" enables the clock output
#define CLOCK_OUTPUT_FLAG 'c'
#define CLOCK_OUTPUT_ENABLE '1'
#define CLOCK_OUTPUT_DISABLE '0'

// Data rate settings: command = "f[datarate]"
// Example: "f0" sets the data rate to 16kHz
#define DATA_RATE_FLAG 'f'
#define DATA_RATE_16K '0'
#define DATA_RATE_8K '1'
#define DATA_RATE_4K '2'
#define DATA_RATE_2K '3'
#define DATA_RATE_1K '4'
#define DATA_RATE_500 '5'
#define DATA_RATE_250 '6'

// Input settings: command = "i[channel][input]"
// Example: "i1n" sets the input of channel 1 to normal
#define INPUT_FLAG 'i'

#define NORMAL_INPUT 'n'
#define SHORTED_INPUT 's'
#define TEST_SIGNAL_INPUT 't'

// Bias settings: command = "b[e[1|0]|r[i|e]|p[e|d][channel]|n[e|d][channel]|m[e|d][channel]]"
// Example: "be1" turns on the bias
// Example: "bri" sets the bias reference to internal
// Example: "bpe1" connects the positive input of channel 1 to the bias
// Example: "bnd2" disconnects the negative input of channel 2 to the bias
// Example: "bme3" enables the bias measurement on channel 3
#define BIAS_FLAG 'b'
#define BIAS_POWER_FLAG 'e'
#define BIAS_POWER_ON '1'
#define BIAS_POWER_OFF '0'
#define BIASREF_FLAG 'r'
#define BIASREF_INTERNAL 'i'
#define BIASREF_EXTERNAL 'e'
#define BIAS_PCHAN_FLAG 'p'
#define BIAS_NCHAN_FLAG 'n'
#define BIAS_CHAN_ENABLE 'e'
#define BIAS_CHAN_DISABLE 'd'
#define BIAS_MEAS_FLAG 'm'
#define BIAS_MEAS_ENABLE 'e'
#define BIAS_MEAS_DISABLE 'd'

// Reference settings: command = "r[SRB1|SRB2][connect|disconnect][chan]"
// Example: "r11" connects the reference to SRB1
// Example: "r205" disconnects SRB2 from channel 5
#define REFERENCE_FLAG 'r'
#define SRB1_FLAG '1'
#define SRB2_FLAG '2'
#define CONNECT_REFERENCE '1'
#define DISCONNECT_REFERENCE '0'

// Test signal settings: command = "t[amp][pulse]"
// Example: "t1f" sets the test signal to 1x amplitude and fast pulse
#define TEST_SIGNAL_FLAG 't'

#define AMP_1X '1'
#define AMP_2X '2'

#define PULSE_SLOW 's'
#define PULSE_FAST 'f'
#define DC_SIGNAL 'd'

// Enable data continuous mode
#define RDATAC_MODE 'z'
#define SDATAC_MODE 'x'

// Single-shot mode: command = "k[0|1]"
// Example: "k1" enters single-shot mode
#define SINGLE_SHOT_MODE 'k'
#define SINGLE_SHOT_ENABLE '1'
#define SINGLE_SHOT_DISABLE '0'

// Start and stop streaming data
#define START_STREAMING 'a'
#define STOP_STREAMING 's'

// Read a single sample
#define SINGLE_READ 'u'

// Read N samples command: command = "n[N]"
#define BATCH_READ 'n'
#define MAX_BATCH_SIZE 500

// Get batch data command: command = "m"
#define GET_BATCH 'm'

// Print all registers
#define PRINT_REGISTERS '?'

// Configure output mode: command = "o[mode]"
// Example: "o0" sets the output mode to text
#define CONFIGURE_OUTPUT_FLAG 'o'
// Modes declaration OUTPUT_TYPE_TEXT (0), OUTPUT_TYPE_BINARY (1), OUTPUT_TYPE_OPENEEG (2)

// Verbose mode: command = "v[0|1]"
// Example: "v0" sets the verbose mode to off
#define VERBOSE_FLAG 'v'


#endif