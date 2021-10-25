# Micro-RTSP-Audio

Library to asynchronously run an RTSP-Server on an ESP32, in order to stream raw audio.

I modified the library Micro-RTSP for video streaming by Kevin Hester: https://github.com/geeksville/Micro-RTSP<br>
Copyright 2018 S. Kevin Hester-Chow, kevinh@geeksville.com (MIT License)

# Usage 

```C++
// set up an Audio source
IAudioSource audioSource = AudioDevice();
// create the Audio Streamer using the audio source
AudioStreamer streamer = AudioStreamer(&audioSource);
    
// create the RTSPServer using the streamer
RTSPServer rtsp = RTSPServer(&streamer);
// start the server asynchronously 
rtsp.runAsync();
```

The Audio Source must implement the IAudioInterface, mainly the function `readDataTo()` which provides the audio data to the streamer.

# Audio Format

Currently the audio format is fixed to L16 (16-bit raw audio) with a fixed sample of 16000.
