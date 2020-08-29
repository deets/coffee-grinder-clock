import sys
import re
import plotly.express as px
import numpy as np
import scipy.fftpack


def main():
    values = []
    with open(sys.argv[1], "rb") as inf:
        for line in inf:
            m = re.match(br".*m: (\d+\.\d+).*", line)
            if m:
                v = float(m.group(1).decode("ascii"))
                values.append(v)

if __name__ == '__main__':
    main()
