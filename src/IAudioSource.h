#pragma once

template<class SAMPLE_TYPE>
class IAudioSource {
    public:
        virtual int getSampleRate() = 0;
        virtual int readDataTo(SAMPLE_TYPE * dest, int maxSamples) = 0;
        virtual void start() {};
        virtual void stop() {};
};