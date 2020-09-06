import sys
import threading
import queue
import http.client
import time
import struct
import argparse

import numpy as np

from scipy.fft import fft
from scipy.signal import hann

from bokeh.models import ColumnDataSource
from bokeh.plotting import curdoc, figure
from bokeh.layouts import column, row
from bokeh.models import FactorRange, Range1d
from bokeh.models import Select

FREQUENCY = 1000.0 # our sampling frequency in the MPU
N = 1024


class Visualisation:

    def __init__(self):
        opts = self._parse_args()

        self._output = lambda x: None
        if opts.output:
            self._output = self._create_recorder(opts.output)

        self._read = self._read_from_http
        if opts.replay:
            self._read = self._create_file_replay(opts.replay)

        self._data_q = queue.Queue()
        doc = self._doc = curdoc()
        self._window = hann(N, sym=True)

        raw_data = dict(
            time=[i for i in range(N)],
            readings=[0.0] * N,
        )

        fft_data = dict(
            frequency=np.linspace(0, FREQUENCY / 2, N // 2),
            fft=[0] * (N // 2),
        )

        self._raw_source = ColumnDataSource(
                data=raw_data,
        )
        raw_figure = figure(
            width=600,
            height=100,
        )
        raw_figure.line(
            x="time",
            y="readings",
            alpha=0.5,
            source=self._raw_source,
        )

        self._fft_source = ColumnDataSource(
                data=fft_data,
        )
        fft_figure = figure(
            width=600,
            height=100,
        )
        fft_figure.line(
            x="frequency",
            y="fft",
            alpha=0.5,
            source=self._fft_source,
        )

        layout = column(
            children=[
                raw_figure,
                fft_figure,
            ],
            sizing_mode="scale_width"
        )
        doc.add_root(layout)

    def _create_recorder(self, filename):
        outf = open(filename, "w")

        def _record(data):
            for value in data:
                outf.write(f"{value}\n")
            outf.flush()
        return _record

    def _parse_args(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("-o", "--output", help="Record data into file")
        parser.add_argument(
            "-r",
            "--replay",
            help="Replay data from recorded file",
        )
        return parser.parse_args()

    def start_background_reader(self):
        t = threading.Thread(target=self._background_task)
        t.daemon = True
        t.start()

    def _background_task(self):
        timestamp = time.monotonic()
        while True:
            data = self._read()
            if data:
                self._data_q.put(data)
                self._doc.add_next_tick_callback(self._process_data)
                self._output(data)
            timestamp += 1/60.0
            time.sleep(max(timestamp - time.monotonic(), 0))

    def _read_from_http(self):
        conn = http.client.HTTPConnection("coffee-grinder-clock.local")
        conn.request("GET", "/")
        response = conn.getresponse()
        data = response.read()
        response.close()
        data = struct.unpack("f" * (len(data) // 4), data)
        return data

    def _create_file_replay(self, filename):
        inf = open(filename, "r")
        timestamp = time.monotonic()

        def _read_from_file():
            nonlocal timestamp
            now = time.monotonic()
            elapsed = (now - timestamp)
            timestamp = now
            lines_to_read = int(FREQUENCY * elapsed)
            print("lines_to_read", lines_to_read, elapsed)
            data = []
            for _ in range(lines_to_read + 1):
                line = inf.readline().strip()
                if line:
                    data.append(float(line))
            return data

        return _read_from_file

    def _process_data(self):
        # For some reason we get multiple callbacks
        # in the mainloop. So instead of relying on
        # one callback per line, we gather them
        # and process as many of them as we find.
        for _ in range(self._data_q.qsize()):
            data = self._data_q.get()[-N:]
            readings = self._raw_source.data['readings']
            readings = readings[len(data):]
            readings.extend(data)

            yf = fft(readings)
            spectrum = yf / N
            autopower = np.abs(spectrum * np.conj(spectrum))
            autopower = autopower[:len(autopower) // 2]
            autopower[:2] = [0.0, 0.0]

            patch = dict(
                readings=[(slice(0, len(readings)), readings)],
            )
            self._raw_source.patch(patch)
            patch = dict(
                fft=[(slice(0, N // 2), autopower)],
            )
            self._fft_source.patch(patch)


def main():
    visualisation = Visualisation()
    visualisation.start_background_reader()


main()