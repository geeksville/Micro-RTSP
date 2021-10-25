#pragma once

#include "IAudioSource.h"
#include <stdint.h>

class AudioTestSource : public IAudioSource<int16_t> {
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
        int readDataTo(int16_t * dest, int maxSamples);
};