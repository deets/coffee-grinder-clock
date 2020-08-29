import sys

import numpy as np

from scipy.fft import fft
from scipy.signal import hann
import matplotlib.pyplot as plt

N = 1024
T = 1.0 / 1000.0


def main():
    window = hann(N, sym=True)
    step = N // 16
    values = []
    for line in open(sys.argv[1]):
        try:
            values.append(float(line))
        except ValueError:
            pass

    ffts = []
    for start in range(0, len(values), step):
        try:
            y = np.array(values[start:start + N])
            y = y * window
        except ValueError:
            break
        yf = fft(y)
        spectrum = yf / N
        autopower = np.abs(spectrum * np.conj(spectrum))
        autopower = autopower[:len(autopower) // 2]
        autopower[:2] = [0.0, 0.0]
        ffts.append(autopower)

    ffts = np.array(ffts)
    ffts = 20 * np.log10(ffts)
    ffts = ffts.T
    plt.imshow(
        ffts,
        origin='lower',
        cmap='jet',
        interpolation='nearest',
        aspect='auto'
    )
    plt.grid()
    plt.show()


if __name__ == '__main__':
    main()
