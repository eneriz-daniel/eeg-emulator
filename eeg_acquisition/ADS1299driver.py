# Copyright (C) 2025 Daniel Enériz
#
# This file is part of EEG Emulation and Acquisition.
#
# EEG Emulation and Acquisition is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# EEG Emulation and Acquisition is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with EEG Emulation and Acquisition.  If not, see <https://www.gnu.org/licenses/>.

from time import sleep

import numpy as np
import serial
from overdue import TaskAbortedError, timecapped_to


class ADS1299:
    """
    A class representing the ADS1299 driver.

    Args:
        port (str): The serial port to communicate with the ADS1299.
        baudrate (int, optional): The baud rate for serial communication.
            Defaults to 115200.

    Attributes:
        channels (list): A list of available channels.
        gains (list): A list of available gains.
        input_modes (list): A list of available input modes.
        test_signal_conf (dict): A dictionary containing the configuration for
            the test signal.
    """

    delay = 0.1  # Delay between commands in seconds
    verbose_header = "ADS1299: "

    valid_channels = [1, 2, 3, 4, 5, 6, 7, 8]
    valid_gains = [0, 1, 2, 4, 6, 8, 12, 24]  # 0 means PGA is bypassed
    valid_data_rates = {
        16000: "0",
        8000: "1",
        4000: "2",
        2000: "3",
        1000: "4",
        500: "5",
        250: "6",
    }
    valid_input_modes = ["normal", "shorted", "test"]
    references = ["SRB1", "SRB2"]
    valid_test_signal_conf = {
        "amplitude": ["1", "2"],
        "frequency": ["slow", "fast", "dc"],
    }

    enabled_channels = [True for _ in range(8)]
    gains = np.ones(8) * 24
    data_rate = 250
    modes = ["shorted" for _ in range(8)]

    NBITS = 24
    VREF = 4.5

    def __init__(self, port, baudrate=115200):
        """
        Initializes the ADS1299driver object.

        Args:
            port (str): The serial port to connect to.
            baudrate (int, optional): The baud rate for the serial communication.
                Defaults to 115200.
        """
        self.ser = serial.Serial(port, baudrate, timeout=1)
        self.ser.flushInput()
        self.ser.flushOutput()

        self.verbosity(True)

    def verbosity(self, value):
        """
        Sets the verbose mode for the ADS1299.

        Args:
            value (bool): True to enable verbose mode, False to disable it.
        """
        if value is True:
            self.ser.write("v1\n".encode())
            self.verbose = True
        else:
            self.ser.write("v0\n".encode())
            self.verbose = False

        if self.verbose:
            print(self.verbose_header + self.read_line())

    def read_line(self) -> str:
        """
        Reads a line from the serial port.

        Returns:
            str: The line read from the serial port.
        """
        return self.ser.readline().decode().strip()

    def set_gain(self, channel, gain):
        """
        Sets the gain for a specific channel.

        Args:
            channel (int): The channel number.
            gain (int): The gain value.

        Raises:
            AssertionError: If the channel or gain is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_gain(1, 24)
            ```
        """
        assert channel in self.valid_channels, (
            "Invalid channel. Available channels: {}".format(self.valid_channels)
        )
        assert gain in self.valid_gains, "Invalid gain. Available gains: {}".format(
            self.valid_gains
        )
        self.ser.write("g{}{:02d}\n".format(channel, gain).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

        if gain == 0:
            self.gains[channel - 1] = 1
        else:
            self.gains[channel - 1] = gain

    def set_gain_for_all(self, gain):
        """
        Sets the gain for all channels.

        Args:
            gain (int): The gain value.

        Raises:
            AssertionError: If the gain is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_gain_for_all(24)
            ```
        """
        assert gain in self.valid_gains, "Invalid gain. Available gains: {}".format(
            self.valid_gains
        )
        for channel in self.valid_channels:
            self.set_gain(channel, gain)

    def set_data_rate(self, data_rate):
        """
        Sets the data rate for the ADS1299.

        Args:
            data_rate (int): The data rate in Hz.

        Raises:
            AssertionError: If the data rate is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_data_rate(1000)
            ```
        """
        assert data_rate in self.valid_data_rates.keys(), (
            "Invalid data rate. Available rates: {}".format(
                self.valid_data_rates.keys()
            )
        )
        self.ser.write("f{}\n".format(self.valid_data_rates[data_rate]).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

        self.data_rate = data_rate

    def set_input_mode(self, channel, mode):
        """
        Sets the input mode for a specific channel.

        Args:
            channel (int): The channel number.
            mode (str): The input mode.

        Raises:
            AssertionError: If the channel or mode is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_input_mode(1, 'normal')
            >>> ads.set_input_mode(2, 'shorted')
            >>> ads.set_input_mode(3, 'test')
            ```
        """
        assert channel in self.valid_channels, (
            "Invalid channel. Available channels: {}".format(self.valid_channels)
        )
        assert mode in self.valid_input_modes, (
            "Invalid input mode. Available modes: {}".format(self.valid_input_modes)
        )
        self.ser.write("i{}{}\n".format(channel, mode[0]).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

        self.modes[channel - 1] = mode

    def set_input_mode_for_all(self, mode):
        """
        Sets the input mode for all channels.

        Args:
            mode (str): The input mode.

        Raises:
            AssertionError: If the mode is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_input_mode_for_all('normal')
            ```
        """
        assert mode in self.valid_input_modes, (
            "Invalid input mode. Available modes: {}".format(self.valid_input_modes)
        )
        for channel in self.valid_channels:
            self.set_input_mode(channel, mode)

    def set_bias(self, mode):
        """
        Enables or disables the bias for the ADS1299.

        Args:
            mode (bool): True to enable the bias, False to disable it.

        Raises:
            AssertionError: If the mode is not a boolean.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_bias(True)
            ```
        """
        assert isinstance(mode, bool), "Invalid mode. Mode must be a boolean."

        self.ser.write("be{}\n".format(int(mode)).encode())

        if self.verbose:
            print(self.verbose_header + self.read_line())

    def set_biasrefint(self, mode):
        """
        Enables or disables the internal bias reference for the ADS1299.

        Args:
            mode (bool): True to enable the internal bias reference, False to
                disable it.

        Raises:
            AssertionError: If the mode is not a boolean.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_biasrefint(True)
            ```
        """
        assert isinstance(mode, bool), "Invalid mode. Mode must be a boolean."

        if mode:
            self.ser.write("bri\n".encode())  # Enable internal bias reference
        else:
            self.ser.write(
                "bre\n".encode()
            )  # Disable internal bias reference, use external

        if self.verbose:
            print(self.verbose_header + self.read_line())

    def set_channel_bias_connections(self, channel, positive, negative):
        """
        Sets the connections of the desired channel to the input of bias drive.

        Args:
            channel (int): The channel number.
            positive (bool): True to connect the positive input to the bias drive.
            negative (bool): True to connect the negative input to the bias drive.

        Raises:
            AssertionError: If the channel is invalid or if positive/negative is
                not a boolean.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_channel_bias_connections(1, True, False)
            >>> ads.set_channel_bias_connections(2, False, True)
            ```
        """
        assert channel in self.valid_channels, (
            "Invalid channel. Available channels: {}".format(self.valid_channels)
        )
        assert isinstance(positive, bool), (
            "Invalid positive mode. Mode must be a boolean."
        )
        assert isinstance(negative, bool), (
            "Invalid negative mode. Mode must be a boolean."
        )

        if positive:
            self.ser.write("bpe{}\n".format(channel).encode())
        else:
            self.ser.write("bpd{}\n".format(channel).encode())
        if self.verbose:
            print(self.verbose_header + self.read_line())

        if negative:
            self.ser.write("bne{}\n".format(channel).encode())
        else:
            self.ser.write("bnd{}\n".format(channel).encode())
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def set_channel_bias_connections_for_all(self, positive, negative):
        """
        Sets the connections of all channels to the input of bias drive.

        Args:
            positive (bool): True to connect the positive inputs to the bias drive.
            negative (bool): True to connect the negative inputs to the bias drive.

        Raises:
            AssertionError: If positive/negative is not a boolean.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_channel_bias_connections_for_all(True, False)
            ```
        """
        for channel in self.valid_channels:
            self.set_channel_bias_connections(channel, positive, negative)

    def bias_measure(self, connect, channel=None):
        """
        Connects the bias to a channel to be measured or disconnects the bias
        measurement.

        Args:
            connect (bool): True to enable bias measurement, False to disable it.
            channel (int, optional): The channel number. Required if connect is True.

        Raises:
            AssertionError: If connect is not a boolean or if the channel is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.bias_measure(True, 1)
            ```
        """
        assert isinstance(connect, bool), (
            "Invalid connect mode. Mode must be a boolean."
        )

        if connect:
            assert channel in self.valid_channels, (
                "Invalid channel. Available channels: {}".format(self.valid_channels)
            )
            self.ser.write("bme{}\n".format(channel).encode())
        else:
            self.ser.write("bmd0\n".encode())

        if self.verbose:
            print(self.verbose_header + self.read_line())

    def set_reference(self, reference, mode, channel=None):
        """
        Sets the references for the ADS1299.

        Args:
            reference (str): The reference to set. Must be 'SRB1' or 'SRB2'.
            mode (bool): True to enable the reference, False to disable it.
            channel (int, optional): The channel number. Required for 'SRB2'.

        Raises:
            AssertionError: If the reference or channel is invalid, or if mode
                is not a boolean.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.set_reference('SRB1', True)
            ```
        """
        assert reference in self.references, (
            "Invalid reference. Available references: {}".format(self.references)
        )
        assert isinstance(mode, bool), "Invalid mode. Mode must be a boolean."

        if reference == "SRB2":
            assert channel in self.valid_channels, (
                "Invalid channel. Available channels: {}".format(self.valid_channels)
            )
        else:
            channel = 0

        self.ser.write("r{}{}{}\n".format(reference[-1], int(mode), channel).encode())

        if self.verbose:
            print(self.verbose_header + self.read_line())

    def enable_channel(self, channel):
        """
        Enables a specific channel.

        Args:
            channel (int): The channel number.

        Raises:
            AssertionError: If the channel is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.enable_channel(1)
            ```
        """
        assert channel in self.valid_channels, (
            "Invalid channel. Available channels: {}".format(self.valid_channels)
        )
        self.ser.write("e{}\n".format(channel).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

        self.enabled_channels[channel - 1] = True

    def enable_all_channels(self):
        """
        Enables all channels.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.enable_all_channels()
            ```
        """
        for channel in self.valid_channels:
            self.enable_channel(channel)

    def disable_channel(self, channel):
        """
        Disables a specific channel.

        Args:
            channel (int): The channel number.

        Raises:
            AssertionError: If the channel is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.disable_channel(1)
            ```
        """
        assert channel in self.valid_channels, (
            "Invalid channel. Available channels: {}".format(self.valid_channels)
        )
        self.ser.write("d{}\n".format(channel).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

        self.enabled_channels[channel - 1] = False

    def disable_all_channels(self):
        """
        Disables all channels.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.disable_all_channels()
            ```
        """
        for channel in self.valid_channels:
            self.disable_channel(channel)

    def rdatac(self):
        """
        Enables Read Data Continuous mode in the ADS1299.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.rdatac()
            ```
        """
        self.ser.write("z\n".encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def sdatac(self):
        """
        Disables Read Data Continuous mode in the ADS1299.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.sdatac()
            ```
        """
        self.ser.write("x\n".encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def start_streaming(self):
        """
        Starts streaming data from the ADS1299.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.start_streaming()
            ```
        """
        self.ser.write("a\n".encode())

    def stop_streaming(self):
        """
        Stops the stream of data from the ADS1299.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.stop_streaming()
            ```
        """
        self.ser.write("s\n".encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def single_shot_mode(self, enable: bool):
        """
        Enables or disables single-shot mode.

        Args:
            enable (bool): True to enable single-shot mode, False to disable it.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.single_shot_mode(True)
            ```
        """
        assert isinstance(enable, bool), "Invalid argument. Mode must be a boolean."
        self.ser.write("k{}\n".format(int(enable)).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def single_read(self) -> np.ndarray:
        """
        Starts a single conversion and reads the data from the ADS1299.

        Returns:
            np.ndarray: A list of data read from the ADS1299.

        Raises:
            TaskAbortedError: If the read operation takes more than its timeout.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> data = ads.single_read()
                [123, 456, 789, ...]
            ```
        """
        self.ser.write("u\n".encode())
        return self.read_ASCII_data()

    def set_batch_read(self, n_samples: int) -> None:
        """
        Sets a read of N samples from the ADS1299.

        Args:
            n_samples (int): The number of samples to read.

        Raises:
            TaskAbortedError: If the read operation takes more than its timeout.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> data = ads.set_batch_read(10)
            ```
        """
        self.batch_size = n_samples
        self.ser.write("n{}\n".format(n_samples).encode())

    def get_batch_read(self) -> np.ndarray:
        """
        Gets the batch data from the ADS1299.

        Returns:
            np.ndarray: A numpy array of data read from the ADS1299.

        Raises:
            TaskAbortedError: If the operation takes more than its timeout.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> data = ads.get_batch_read()
                [[123, 456, 789, ...], [123, 456, 789, ...], ...]
            ```
        """
        if self.batch_size is None:
            raise ValueError("Batch size not set. Call read_batch() first.")

        data = []

        self.ser.flush()

        self.ser.write("m\n".encode())

        for i in range(self.batch_size):
            data.append(self.read_ASCII_data())

        data = np.array(data).T

        self.batch_size = None  # Reset batch size after reading

        return data

    def close(self):
        """
        Closes the serial connection.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.close()
            ```
        """
        self.ser.close()

    @timecapped_to(1)
    def read_ASCII_data(self, normalized=True) -> np.array:
        """
        Reads data from the ADS1299.

        Args:
            normalized (bool): True to normalize the data, False to return raw data.

        Returns:
            np.array: A numpy array of data read from the ADS1299.

        Raises:
            TaskAbortedError: If the operation takes more than its timeout.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> data = ads.read_ASCII_data(True)
                [0.123, 0.456, 0.789, ...]
            ```
        """
        data = self.ser.readline().decode().strip()
        data = data.split(",")[:-1]
        # data = [int(d) for d in data]
        data = np.array(data, dtype=int)

        if normalized:
            data = self._norm(data)

        return data

    def test_signal(self, amplitude, frequency):
        """
        Configures the test signal to the ADS1299.

        Args:
            amplitude (str): The amplitude of the test signal.
            frequency (str): The frequency of the test signal.

        Raises:
            AssertionError: If the amplitude or frequency is invalid.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.test_signal('1', 'fast')
            ```
        """
        assert amplitude in self.valid_test_signal_conf["amplitude"], (
            "Invalid amplitude. Available amplitudes: {}".format(
                self.valid_test_signal_conf["amplitude"]
            )
        )
        assert frequency in self.valid_test_signal_conf["frequency"], (
            "Invalid frequency. Available frequencies: {}".format(
                self.valid_test_signal_conf["frequency"]
            )
        )
        self.ser.write("t{}{}\n".format(amplitude, frequency[0]).encode())
        sleep(self.delay)
        if self.verbose:
            print(self.verbose_header + self.read_line())

    def log_data(self, filename, duration=10):
        """
        Logs data from the ADS1299 to a file.

        Args:
            filename (str): The name of the file to log the data to.
            duration (int, optional): The duration of the logging in seconds.
                Defaults to 10. If set to -1, the logging will continue
                indefinitely.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.log_data('data_log.txt', 10)
            ```
        """
        from time import strftime, time

        f = open(filename, "w")
        f.write("ADS1299 data log @ {}\n".format(strftime("%Y-%m-%d %H:%M:%S")))

        start_time = time()
        while time() - start_time < duration or duration == -1:
            try:
                data = self.read_ASCII_data(True)
            except TaskAbortedError:
                print("Read operation timed out.")
                break
            f.write(", ".join([str(d) for d in data]) + "\n")
            f.flush()  # Flush the buffer to write to the file

        f.close()

    def print_all_registers(self):
        """
        Prints all registers from the ADS1299.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.print_all_registers()
            ID, 0x00, 0x3E, 0, 0, 1, 1, 1, 1, 1, 0
            CONFIG1, 0x01, 0x96, 1, 0, 0, 1, 0, 1, 1, 0
            CONFIG2, 0x02, 0xC0, 1, 1, 0, 0, 0, 0, 0, 0
            CONFIG3, 0x03, 0xE0, 1, 1, 1, 0, 0, 0, 0, 0
            LOFF, 0x04, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            CH1SET, 0x05, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH2SET, 0x06, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH3SET, 0x07, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH4SET, 0x08, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH5SET, 0x09, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH6SET, 0x0A, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH7SET, 0x0B, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            CH8SET, 0x0C, 0x61, 0, 1, 1, 0, 0, 0, 0, 1
            BIAS_SENSP, 0x0D, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            BIAS_SENSN, 0x0E, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            LOFF_SENSP, 0x0F, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            LOFF_SENSN, 0x10, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            LOFF_FLIP, 0x11, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            LOFF_STATP, 0x12, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            LOFF_STATN, 0x13, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            GPIO, 0x14, 0x0F, 0, 0, 0, 0, 1, 1, 1, 1
            MISC1, 0x15, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            MISC2, 0x16, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            CONFIG4, 0x17, 0x00, 0, 0, 0, 0, 0, 0, 0, 0
            ```
        """
        N_REGISTERS = 0x18
        self.ser.write("?\n".encode())
        for i in range(N_REGISTERS):
            print(self.read_line())

    def _norm(self, data):
        """
        Normalizes the data read from the ADS1299, converting it to volts.

        Args:
            data (np.array): The integer values read from the ADS1299.

        Returns:
            np.array: The normalized data in volts.
        """
        norm_data = data / (2 ** (self.NBITS - 1) - 1) * self.VREF / self.gains

        return norm_data

    def live_plot(self, buffer_width=1000):
        """
        Creates a live plot of the 8 channels' data over the last `width` seconds.

        Args:
            buffer_width (int): The time window to display in the plot.

        Example:
            ```python
            >>> ads = ADS1299('COM8')
            >>> ads.live_plot()
            ```
        """

        import pyqtgraph as pg


        app = pg.mkQApp("Plotting Example")

        win = pg.GraphicsLayoutWidget(show=True, title="EEG acquisition - ADS1299")
        win.resize(1000,600)
        win.setWindowTitle("ADS1299 - EEG acquisition")

        pg.setConfigOptions(antialias=True)

        p = []
        curve = []
        for i in range(len(self.valid_channels)):
            p.append(win.addPlot())
            p[i].setLabel('left', 'Chan{}'.format(i), units='V')
            # Remove labels and ticks in x axis
            p[i].setLabel('bottom', '')
            p[i].getAxis('bottom').setTicks([])
            curve.append(p[i].plot())
            win.nextRow()

        ADC_values = np.zeros((len(self.valid_channels), buffer_width))

        def update():
            ADC_values[:,:-1] = ADC_values[:, 1:]
            data = ads.single_read()[:len(self.valid_channels)]
            ADC_values[:,-1] =  data
            for i in range(len(self.valid_channels)):
                curve[i].setData(ADC_values[i,:])

            app.processEvents()

        try:
            while True:
                update()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    ads = ADS1299("COM8")

    ads.set_reference("SRB1", True)
    ads.set_gain_for_all(24)
    ads.enable_all_channels()
    ads.set_input_mode_for_all("normal")

    ads.live_plot(buffer_width=1000)
