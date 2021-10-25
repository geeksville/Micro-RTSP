#pragma once

class IAudioSource {
    public:
        virtual int getSampleRate() = 0;
        virtual int readDataTo(unsigned char * dest, int maxBytes) = 0;
};