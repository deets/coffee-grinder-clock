# -*- coding: utf-8 -*-
# Copyright: 2020, Diez B. Roggisch, Berlin . All rights reserved.
import time
import http.client


def main():

    while True:
        conn = http.client.HTTPConnection("coffee-grinder-clock.local")
        conn.request("GET", "/")
        response = conn.getresponse()
        print(response.status, response.reason)
        data = response.read()
        response.close()
        time.sleep(1)


if __name__ == '__main__':
    main()
