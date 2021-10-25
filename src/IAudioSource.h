/*
 * Author: Thomas Pfitzinger
 * github: https://github.com/Tomp0801/Micro-RTSP-Audio
 *
 * Based on Micro-RTSP library for video streaming by Kevin Hester:
 * 
 * https://github.com/geeksville/Micro-RTSP
 * 
 * Copyright 2018 S. Kevin Hester-Chow, kevinh@geeksville.com (MIT License)
 */

#pragma once

/**
 * Interface for an audio source for an RTP stream
 */
class IAudioSource {
    public:
        /**
         * Returns sample rate at which data is provided
         * @return the sample rate
         */
        virtual int getSampleRate() = 0;
        /**
         * Returns size of a single sample in bytes
         * @return sample size in bytes
         */
        virtual int getSampleSizeBytes() = 0;
        /**
         * (Reads and) Copies up to maxSamples samples into the given buffer
         * @param dest Buffer into which the samples are to be copied
         * @param maxSamples maximum number of samples to be copied
         * @return actual number of samples that were copied
         */
        virtual int readDataTo(void * dest, int maxSamples) = 0;
        /**
         * Start preparing data in order to provide it for the stream
         */
        virtual void start() {};
        /**
         * Stop preparing data as the stream has ended
         */ 
        virtual void stop() {};
};