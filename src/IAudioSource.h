#pragma once

class IAudioSource {
    public:
        virtual int getSampleRate() = 0;
        virtual int getSampleSizeBytes() = 0;
        virtual int readDataTo(void * dest, int maxSamples) = 0;
        virtual void start() {};
        virtual void stop() {};
};