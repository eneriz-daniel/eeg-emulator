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

import matplotlib.pyplot as plt
import nidaqmx
import numpy as np


class EEGMux:
    def __init__(self, N_CHAN=8, T=3, ds=1, fs=160):
        self.N_CHAN = N_CHAN
        self.T = T
        self.ds = ds
        self.fs = fs

        # Compute the sampling frequency of the multiplexed signal for the DAQ
        self.fg = fs / ds * N_CHAN

        # Compute the samples
        self.N_SAMPLES = int(T * fs * N_CHAN / ds)

        self._set_counter_parallel_load()

        self._init_start_trigger()
        self._init_clk()
        self._init_ao()

    def _set_counter_parallel_load(self):
        rst = nidaqmx.Task()
        rst.do_channels.add_do_chan("Dev1/pfi1")
        rst.write(False)
        sleep(0.01)
        rst.write(True)
        rst.close()

    def _init_start_trigger(self):
        self.start_trigger = nidaqmx.Task()
        self.start_trigger.do_channels.add_do_chan("Dev1/pfi2")
        self.start_trigger.write(False)

    def _init_clk(self):
        self.clk = nidaqmx.Task()
        self.clk.co_channels.add_co_pulse_chan_freq(
            "Dev1/ctr0", freq=self.fg, units=nidaqmx.constants.FrequencyUnits.HZ,
            initial_delay=0.01925, # seconds
                # 25ms of t_settle at DR 250 Sa/s with CLK 1.310720 MHz minus
                # 5.5 ms of 7 cycles at 1280 Hz plus 250us to make the conversion
                # when chan 7 is stable
            idle_state=nidaqmx.constants.Level.LOW,
        )
        self.clk.timing.cfg_implicit_timing(
            sample_mode=nidaqmx.constants.AcquisitionType.FINITE,
            samps_per_chan=self.N_SAMPLES,
        )
        self.clk.triggers.start_trigger.cfg_dig_edge_start_trig("PFI2")

    def _init_ao(self):
        self.ao = nidaqmx.Task()
        self.ao.ao_channels.add_ao_voltage_chan("Dev1/ao0", min_val=0, max_val=5)
        self.ao.timing.cfg_samp_clk_timing(
            rate=self.fg,
            source="PFI0",
            sample_mode=nidaqmx.constants.AcquisitionType.FINITE,
            samps_per_chan=self.N_SAMPLES,
        )

    def start(self):
        self.ao.start()
        self.clk.start()
        self.start_trigger.write(True)

    def wait_until_done(self):
        self.clk.wait_until_done()

    def stop(self):
        self.start_trigger.write(False)
        self.start_trigger.stop()
        self.ao.stop()
        self.clk.stop()

    def close(self):
        self.start_trigger.close()
        self.ao.close()
        self.clk.close()


def scale(x, scal_factor, offset):
    return x * scal_factor + offset


if __name__ == "__main__":

    # Script below is to test the EEG demux board by acquiring the amplified
    # EEG signal (AAMP pins) with the DAQ

    # EEG settings
    fs = 160
    ds = 1
    T = 3
    N_CHAN = 8

    # Load EEG
    with np.load("dataset_Nchan64_T3_ds1_Nclass4_normFalse.npz") as data:
        x_orig = data["X"]

    # Select sample
    N_SAMPLE = 788  # np.random.randint(0, x.shape[0])

    # Select channels and sample
    x_orig = x_orig[N_SAMPLE, :N_CHAN, :]

    # Original data is in the ±1 mV, and the PCB expects it to be in the 0-5 V range
    x = scale(x_orig, 2500, 2.5)

    eegmux = EEGMux()

    ai = nidaqmx.Task()
    ai.ai_channels.add_ai_voltage_chan(
        "Dev1/ai0:7", # Connected to the AAMP pins
        min_val=-5,
        max_val=5,
        terminal_config=nidaqmx.constants.TerminalConfiguration.RSE,
    )
    ai.timing.cfg_samp_clk_timing(
        rate=fs,
        sample_mode=nidaqmx.constants.AcquisitionType.FINITE,
        samps_per_chan=T * fs,
    )
    ai.triggers.start_trigger.cfg_dig_edge_start_trig("PFI2")

    eegmux.ao.write(x.flatten("F"))

    input("Press enter to start the EEG mux...")

    eegmux.start()
    data = ai.read(number_of_samples_per_channel=T * fs)

    eegmux.stop()
    ai.stop()
    eegmux.close()
    ai.close()

    data = np.array(data)

    # Remove last two samples
    data = data[:, :-2]

    # Add two data[i, 0] points to the beginning of the data
    first_vals = data[:, 0]
    data = np.insert(data, 0, first_vals, axis=1)
    data = np.insert(data, 0, first_vals, axis=1)

    print(data.shape)

    fig, axs = plt.subplots(N_CHAN, 1, sharex=True)

    for i in range(N_CHAN):
        ax_tw = axs[i].twinx()
        ax_tw.plot(x_orig[i, :], color="C0", alpha=0.5)
        axs[i].plot(data[i, :], color="C1")
        axs[i].set_ylabel(f"Ch{i}")

    axs[-1].set_xlabel("Samples")

    # Fake plot for the legend
    axs[0].plot([], color="C0", label="Original")
    axs[0].plot([], color="C1", label="Acquired (AAMP)")

    fig.legend()

    plt.show()

    np.savez("eeg_mux_test.npz", x_orig=x_orig, x=x, data=data)
