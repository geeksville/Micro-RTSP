# Testserver

This is a standalone Linux test application to allow development of this
library without going through the slow process of always testing on the ESP32.
Almost all of the code is the same - only platglue-posix.h differs from
platglue-esp32.h (thus serving as a crude HAL).

RESPTestServer.cpp also serves as a small example of how this library could
be used on Poxix systems.

# Usage

Run "make" to build and run the server.  Run "runvlc.sh" to fire up a VLC client
that talks to that server.  If all is working you should see a static image
of my office that I captured using a ESP32-CAM.
