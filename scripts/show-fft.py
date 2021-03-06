# -*- coding: utf-8 -*-
# Copyright: 2020, Diez B. Roggisch, Berlin . All rights reserved.
import sys

import numpy as np
import matplotlib.pyplot as plt

from common import load_fft_data

N = 1024
T = 1.0 / 1000.0

NOTCHES = [
    (253, 259),
    (285, 290),
    (464, 472),
]

START = 250
END = 2010
THRESHOLD = -170.0


def main():
    ffts = load_fft_data(sys.argv[1])
    # crop to relevant range
    ffts = ffts[START:END, :]
    # lowpass
    ffts[:, :20] = 0.0
    for low, high in NOTCHES:
        ffts[:, low:high] = 0.0

    ffts = 20 * np.log10(ffts)

    if THRESHOLD is not None:
        ffts[ffts < THRESHOLD] = 0.0

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
