# Micro-RTSP

This is a small library which can be used to serve up RTSP streams from
resource constrained MCUs.

Note: This is still a WIP but it is working pretty good on my little ESP32.
I should have an initial version out by about Feb 5th...  That version will include more documentation.

# Usage

This library works for ESP32/arduino targets but also for most any posixish platform.

## Example arduino/ESP32 usage

## Example posix/linux usage

There is a small standalone example [here](/test/RTSPTestServer.cpp).  You can build it by following [these](/test/README.md) directions.

# Structure and design notes

# Issues and sending pull requests

Please report issues and send pull requests.  I'll happily reply. ;-)

# Credits

The server code was initially based on a great 2013 [tutorial](https://www.medialan.de/usecase0001.html) by Medialan.

# License

Copyright 2018 S. Kevin Hester-Chow, kevinh@geeksville.com (MIT License)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
