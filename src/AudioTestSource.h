#pragma once

#include "IAudioSource.h"
#include <stdint.h>

class AudioTestSource : public IAudioSource {
    private:
        int index = 0;
        const int testDataSamples = 1024;
        static bool converted;

    public:
        static int16_t testData[];
        static const int sampleRate = 16000;


    public:
        AudioTestSource();
        int getSampleRate();
        int getSampleSizeBytes() { return sizeof(int16_t); };
        int readDataTo(void * dest, int maxSamples);
};