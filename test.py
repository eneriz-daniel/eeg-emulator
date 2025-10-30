import matplotlib.pyplot as plt
import numpy as np

from eeg_acquisition.ADS1299driver import ADS1299
from eeg_emulation.daq import EEGMux, scale

DATASET_PATH = "eeg_emulation/dataset_Nchan8_T3_ds1_Nclass4_normFalse.npy"
# Generated with https://github.com/eneriz-daniel/MIBCI-QCNNs/blob/master/utils/get_data.py

if __name__ == "__main__":

    ads = ADS1299("COM8") # Change to your port

    ads.set_data_rate(250)
    ads.set_reference("SRB1", False)
    ads.set_gain_for_all(24)
    ads.enable_all_channels()
    ads.set_input_mode_for_all("normal")

    ads.print_all_registers()

    input("Press Enter to continue...")

    # EEG settings
    fs = 160
    ds = 1
    T = 3
    N_CHAN = 8

    # Load EEG
    x_orig = np.load(DATASET_PATH)

    # Select channels and sample
    x_orig = x_orig[788, :, :]
    # Original data is in the ±1 mV, and the PCB expects it to be in the 0-5 V range
    x = scale(x_orig, 2500, 2.5)

    eegmux = EEGMux()

    eegmux.ao.write(x.flatten("F"))

    ads.rdatac()
    ads.set_batch_read(x.shape[1])

    try:
        eegmux.start()
        eegmux.wait_until_done()

    except Exception as e:
        print("An error occurred during acquisition.")
        raise e

    finally:
        # Close the connection
        eegmux.stop()
        eegmux.close()

    data = ads.get_batch_read()

    ads.sdatac()
    ads.close()

    # Invert channels
    data = data[::-1, :]

    data = data[:, 2:-1]
    data = np.concatenate((data[:, 0:1], data, data[:, -1:], data[:, -1:]), axis=1)

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
    axs[0].plot([], color="C1", label="Acquired")

    fig.legend()

    plt.show()
