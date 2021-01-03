# Copyright: 2020, Diez B. Roggisch, Berlin . All rights reserved.
import argparse

import numpy as np
from bokeh.plotting import figure, output_file, show

from common import load_fft_data


TOOLTIPS = [
    ("(x,y)", "($x, $y)"),
]

def parse_band_spec(band):
    return int(band)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("fft_data")
    parser.add_argument("bands", nargs="+")
    parser.add_argument("--db", action="store_true")
    parser.add_argument("--start", type=int)
    parser.add_argument("--stop", type=int)
    opts = parser.parse_args()
    ffts = load_fft_data(opts.fft_data)

    if opts.db:
        ffts = 20 * np.log10(ffts)

    if opts.start is not None or opts.stop is not None:
        ffts = ffts[slice(opts.start, opts.stop), :]

    p = figure(title="fft bands", tooltips=TOOLTIPS)
    x = np.linspace(0, ffts.shape[0] - 1, ffts.shape[0])

    for band in opts.bands:
        band = parse_band_spec(band)
        p.line(x, ffts[:, band], legend_label=f"band={band}")

    p.legend.click_policy = "hide"

    show(p)


if __name__ == '__main__':
    main()
