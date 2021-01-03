import sys

import numpy as np

import matplotlib.pyplot as plt
from scipy.fft import fft
from scipy.signal import hann

N = 256
STEP = 16 # we sample 1 / ms, and move with 16ms steps on the ESPs
HIGHPASS = 1


def read_floats(fname):
    values = []
    with open(fname) as inf:
        for line in inf:
            try:
                values.append(float(line))
            except ValueError:
                pass
    return values


def main():
    # start, end = 5.0, 6.0
    values = read_floats(sys.argv[1])
    y = np.array(values, dtype=np.float32)
    x = np.linspace(0.0, len(values) / 1000.0, len(values))
    # slice = np.bitwise_and(x >= start, x <= end)
    # x = x[slice]
    # y = y[slice]
    plt.subplot(311)
    plt.plot(x, y)
    plt.grid(True)

    window = np.array(hann(N, sym=False), dtype=np.float32)
    ffts = []
    for n in range(0, len(y), STEP):
        part = y[n:n+N]
        if len(part) != N:
            continue # last part
        spectrum = (fft(part * window) / len(part))
        # highpass
        spectrum[0:HIGHPASS] = [0] * HIGHPASS
        autopower = np.abs(spectrum * np.conj(spectrum))[:N // 2]
        decibel = 20 * np.log10(autopower)
        ffts.append(decibel)

    plt.subplot(312)
    ffts = np.array(ffts)
    ffts = ffts.T
    plt.imshow(
        ffts,
        origin='lower',
        cmap='jet',
        interpolation='nearest',
        aspect='auto'
    )

    ffts = []
    with open(sys.argv[2]) as inf:
        for line in inf:
            line = line.strip()
            if line:
                part = [float(v) for v in line.split(",")]
                part[0:HIGHPASS] = [0] * HIGHPASS
                decibel = 20 * np.log10(part)
                ffts.append(decibel[:N//2])

    plt.subplot(313)
    ffts = np.array(ffts)
    ffts = ffts.T
    plt.imshow(
        ffts,
        origin='lower',
        cmap='jet',
        interpolation='nearest',
        aspect='auto'
    )

    plt.grid(True)
    plt.show()

if __name__ == '__main__':
    main()
