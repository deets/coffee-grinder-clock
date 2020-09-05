import threading
import queue
import http.client
import time
import struct

from bokeh.models import ColumnDataSource
from bokeh.plotting import curdoc, figure
from bokeh.layouts import column, row
from bokeh.models import FactorRange, Range1d
from bokeh.models import Select


class Visualisation:

    def __init__(self):
        self._data_q = queue.Queue()
        doc = self._doc = curdoc()

        raw_data = dict(
            time=[i for i in range(1000)],
            readings=[0.0] * 1000,
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

        layout = column(
            children=[
                raw_figure,
            ],
            sizing_mode="scale_width"
        )
        doc.add_root(layout)

    def start_background_reader(self):
        t = threading.Thread(target=self._background_task)
        t.daemon = True
        t.start()

    def _background_task(self):
        while True:
            conn = http.client.HTTPConnection("coffee-grinder-clock.local")
            conn.request("GET", "/")
            response = conn.getresponse()
            data = response.read()
            response.close()
            data = struct.unpack("f" * (len(data) // 4), data)
            self._data_q.put(data)
            self._doc.add_next_tick_callback(self._process_data)
            time.sleep(1/60.0)

    def _process_data(self):
        # For some reason we get multiple callbacks
        # in the mainloop. So instead of relying on
        # one callback per line, we gather them
        # and process as many of them as we find.
        for _ in range(self._data_q.qsize()):
            data = self._data_q.get()
            readings = self._raw_source.data['readings']
            readings = readings[len(data):]
            readings.extend(data)
            patch = dict(readings=[(slice(0, len(readings)), readings)])
            self._raw_source.patch(patch)


def main():
    visualisation = Visualisation()
    visualisation.start_background_reader()


main()
