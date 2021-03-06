import threading
import queue
import http.client
import time
import struct
import argparse
from math import isnan

import numpy as np

from scipy.fft import fft
from scipy.signal import hann

from bokeh.models import ColumnDataSource
from bokeh.plotting import curdoc, figure
from bokeh.layouts import column, row
from bokeh.models import FactorRange, Range1d
from bokeh.models import Select

FREQUENCY = 1000.0 # our sampling frequency in the MPU
N = 512
HIGHPASS = 4


class Visualisation:

    def __init__(self):
        opts = self._parse_args()

        self._data_q = queue.Queue()
        doc = self._doc = curdoc()

        self._conn = http.client.HTTPConnection("coffee-grinder-clock.local")

        self._output = lambda x: None
        if opts.output:
            self._output = self._create_recorder(opts.output)

        self._raw_read = self._raw_read_from_http
        if opts.replay:
            self._raw_read = self._create_file_replay(opts.replay)

        raw_data = dict(
            time=[i for i in range(N)],
            readings=[0.0] * N,
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

        children = [raw_figure]
        self._process_fft = not opts.no_fft

        if self._process_fft:
            self._window = hann(N, sym=True)

            self._fft_output = lambda x: None
            if opts.fft:
                self._fft_output = self._create_recorder(opts.fft, "--")

            self._fft_read = self._read_fft_from_http
            if opts.replay_fft:
                self._fft_read = self._create_file_replay(opts.replay_fft)


            fft_data = dict(
                frequency=np.linspace(0, FREQUENCY / 2, N // 2),
                fft=[0] * (N // 2),
            )

            esp_fft_data = dict(
                frequency=np.linspace(0, FREQUENCY / 2, N // 2),
                fft=[0] * (N // 2),
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

            self._esp_fft_source = ColumnDataSource(
                    data=esp_fft_data,
            )
            esp_fft_figure = figure(
                width=600,
                height=100,
            )
            esp_fft_figure.line(
                x="frequency",
                y="fft",
                alpha=0.5,
                source=self._esp_fft_source,
            )
            children.extend([fft_figure, esp_fft_figure])

        layout = column(
            children,
            sizing_mode="scale_width"
        )
        doc.add_root(layout)

    def _create_recorder(self, filename, separator=None):
        outf = open(filename, "w")

        def _record(data):
            for value in data:
                outf.write(f"{value}\n")
            if separator is not None:
                outf.write(f"{separator}\n")
            outf.flush()
        return _record

    def _parse_args(self):
        parser = argparse.ArgumentParser()
        parser.add_argument("--no-fft", action="store_true", help="No FFT processing at all")
        parser.add_argument("-o", "--output", help="Record data into file")
        parser.add_argument("-f", "--fft", help="Record FFT data into file")
        parser.add_argument(
            "-r",
            "--replay",
            help="Replay raw data from recorded file",
        )
        parser.add_argument(
            "--replay-fft",
            help="Replay FFT data from recorded file",
        )
        return parser.parse_args()

    def start_background_reader(self):
        t = threading.Thread(target=self._background_task)
        t.daemon = True
        t.start()

    def _background_task(self):
        timestamp = time.monotonic()
        while True:
            data = self._raw_read()
            next_tick = False
            if data:
                self._data_q.put(("raw", data))
                self._output(data)
                next_tick = True

            if self._process_fft:
                data = self._fft_read()
                if data:
                    self._data_q.put(("fft", data))
                    self._fft_output(data)

            if next_tick:
                self._doc.add_next_tick_callback(self._process_data)

            timestamp += 1/60.0
            time.sleep(max(timestamp - time.monotonic(), 0))

    def _raw_read_from_http(self):
        self._conn.request("GET", "/")
        response = self._conn.getresponse()
        data = response.read()
        response.close()
        data = struct.unpack("f" * (len(data) // 4), data)
        return data

    def _read_fft_from_http(self):
        self._conn.request("GET", "/fft")
        response = self._conn.getresponse()
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
            kind, data = self._data_q.get()[-N:]
            if kind == "raw":
                self._process_raw_data(data)
            elif kind == "fft":
                self._process_fft_data(data)

    def _process_raw_data(self, data):
        readings = self._raw_source.data['readings']
        # append the new readings to the existing array
        readings = readings[len(data):]
        readings.extend(data)
        readings = np.array(readings, dtype=np.single)

        patch = dict(
            readings=[(slice(0, len(readings)), readings)],
        )
        self._raw_source.patch(patch)

        if self._process_fft:
            yf = fft(readings)
            spectrum = yf / N
            complex_ = spectrum * np.conj(spectrum)
            autopower = np.abs(complex_)
            autopower = self._process_fft_output(autopower)
            patch = dict(
                fft=[(slice(0, N // 2), autopower)],
            )
            self._fft_source.patch(patch)

    def _process_fft_output(self, autopower):
        autopower = autopower[:len(autopower) // 2]
        autopower[:HIGHPASS] = [0.0] * HIGHPASS
        autopower = 20 * np.log10(autopower)
        return autopower

    def _process_fft_data(self, data):
        data = [v if not isnan(v) else -1 for v in data]
        data = np.array(data)
        # forget about the upper half, it's garbage
        fft_data = data[:N]
        autopower = self._process_fft_output(fft_data)
        patch = dict(
            fft=[(slice(0, N // 2), autopower)],
        )
        self._esp_fft_source.patch(patch)


def main():
    visualisation = Visualisation()
    visualisation.start_background_reader()


main()
