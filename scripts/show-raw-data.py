import sys

import numpy as np

import matplotlib.pyplot as plt
from scipy.fft import fft
from scipy.signal import hann

def main():
    start, end = 5.0, 6.0
    values = []
    for line in open(sys.argv[1]):
        try:
            values.append(float(line))
        except ValueError:
            pass
    y = np.array(values)
    x = np.linspace(0.0, len(values) / 1000.0, len(values))
    slice = np.bitwise_and(x >= start, x <= end)
    # x = x[slice]
    # y = y[slice]
    plt.subplot(211)
    plt.plot(x, y)
    plt.grid(True)

    window = hann(len(y), sym=False)
    spectrum = fft(y * window) / len(y)
    spectrum[0:2] = [0, 0]
    autopower = np.abs(spectrum * np.conj(spectrum))[:len(y) // 2]
    decibel = 20 * np.log10(autopower)
    plt.subplot(212)
    plt.plot(
        np.linspace(0, len(decibel), len(decibel)),
        decibel
    )
    plt.grid(True)

    plt.show()


if __name__ == '__main__':
    main()
