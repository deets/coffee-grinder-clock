import sys

import serial


def main():
    conn = serial.Serial(sys.argv[1], 115200)
    while True:
        print(conn.read())


if __name__ == '__main__':
    main()
