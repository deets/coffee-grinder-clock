# -*- coding: utf-8 -*-
# Copyright: 2020, Diez B. Roggisch, Berlin . All rights reserved.
import sys
import time
import http.client
import struct


def read_floats(conn, path):
    print(path)
    conn.request("GET", path)
    response = conn.getresponse()
    data = response.read()
    response.close()
    return struct.unpack("f" * (len(data) // 4), data)


def main():
    conn = http.client.HTTPConnection("coffee-grinder-clock.local")
    print("connected")
    with open(sys.argv[1], "w") as rawf, open(sys.argv[2], "w") as fftf:
        while True:
            raw_data = read_floats(conn, "/")
            fft_data = read_floats(conn, "/fft")
            print("raw", len(raw_data), "fft: ", len(fft_data))
            rawf.write("\n".join(str(f) for f in raw_data))
            rawf.write("\n")
            fftf.write(",".join(str(f) for f in fft_data))
            fftf.write("\n")

            time.sleep(0.016)

if __name__ == '__main__':
    main()
