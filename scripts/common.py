# -*- coding: utf-8 -*-
# Copyright: 2020, Diez B. Roggisch, Berlin . All rights reserved.
import numpy as np


def load_fft_data(fname):
    ffts = []
    with open(fname) as inf:
        current_fft = []
        for line in inf:
            line = line.strip()
            if line == "--":
                ffts.append(current_fft[:512])
                current_fft = []
            else:
                current_fft.append(float(line))

    ffts = np.array(ffts)
    return ffts
