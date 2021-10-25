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

#include "AudioStreamer.h"
#include <stdio.h>


AudioStreamer::AudioStreamer()
{
    log_v("Creating Audio streamer");
    m_RtpServerPort  = 0;
    m_RtcpServerPort = 0;

    m_SequenceNumber = 0;
    m_Timestamp      = 0;
    m_SendIdx        = 0;

    m_RtpSocket = NULLSOCKET;
    m_RtcpSocket = NULLSOCKET;

    m_prevMsec = 0;

    m_udpRefCount = 0;

    RtpBuf = new unsigned char[STREAMING_BUFFER_SIZE];

    esp_timer_create_args_t timer_args;
    timer_args.callback = AudioStreamer::doRTPStream;
    timer_args.name = "RTP_timer";
    timer_args.arg = (void*)this;
    timer_args.dispatch_method = ESP_TIMER_TASK;

    esp_timer_init();
    esp_timer_create(&timer_args, &RTP_timer);
    
    this->m_fragmentSize = m_samplingRate / 50;
    this->m_fragmentSizeBytes = m_fragmentSize * m_sampleSizeBytes;

    log_i("Audio streamer created. Sampling rate: %i, Fragment size: %i (%i bytes)", m_samplingRate, m_fragmentSize, m_fragmentSizeBytes);
}

AudioStreamer::AudioStreamer(IAudioSource * source) : AudioStreamer() {
    this->m_audioSource = source;
    this->m_samplingRate = source->getSampleRate();
    this->m_sampleSizeBytes = source->getSampleSizeBytes();
}

AudioStreamer::~AudioStreamer()
{
    delete RtpBuf;
}

int AudioStreamer::getSampleRate() {
    return this->m_samplingRate;
}

int AudioStreamer::SendRtpPacketDirect() {

    // Prepare the 12 byte RTP header TODO this can be optimized, some is static
    RtpBuf[0]  = 0x80;                               // RTP version
    RtpBuf[1]  = 0x0b | 0x80;                               // L16 payload (11) and no marker bit
    RtpBuf[3]  = m_SequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
    RtpBuf[2]  = m_SequenceNumber >> 8;
    RtpBuf[4]  = (m_Timestamp & 0xFF000000) >> 24;   // each image gets a timestamp
    RtpBuf[5]  = (m_Timestamp & 0x00FF0000) >> 16;
    RtpBuf[6] = (m_Timestamp & 0x0000FF00) >> 8;
    RtpBuf[7] = (m_Timestamp & 0x000000FF);
    RtpBuf[8] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
    RtpBuf[9] = 0xf9;                               // we just an arbitrary number here to keep it simple
    RtpBuf[10] = 0x7e;
    RtpBuf[11] = 0x67;

    // append data to header
    if (m_audioSource == NULL) {
        log_e("No audio source provided");
        return -1;
    }
    //unsigned char * dataBuf = ((unsigned char*)RtpBuf + HEADER_SIZE);
    unsigned char * dataBuf = &RtpBuf[HEADER_SIZE];
    
    int samples = m_audioSource->readDataTo((void*)dataBuf, m_fragmentSize);
    int byteLen = samples * m_audioSource->getSampleSizeBytes();

    m_SequenceNumber++;                              // prepare the packet counter for the next packet

    udpsocketsend(m_RtpSocket, RtpBuf, HEADER_SIZE + byteLen, m_ClientIP, m_ClientPort);

    return samples;
}

u_short AudioStreamer::GetRtpServerPort()
{
    return m_RtpServerPort;
};

u_short AudioStreamer::GetRtcpServerPort()
{
    return m_RtcpServerPort;
};

bool AudioStreamer::InitUdpTransport(IPADDRESS aClientIP, IPPORT aClientPort)
{
    m_ClientIP = aClientIP;
    m_ClientPort = aClientPort;

    m_SequenceNumber = getRandom();

    if (m_udpRefCount != 0)
    {
        ++m_udpRefCount;
        return true;
    }

    for (u_short P = 6970; P < 0xFFFE; P += 2)
    {
        m_RtpSocket     = udpsocketcreate(P);
        if (m_RtpSocket)
        {   // Rtp socket was bound successfully. Lets try to bind the consecutive Rtsp socket
            m_RtcpSocket = udpsocketcreate(P + 1);
            if (m_RtcpSocket)
            {
                m_RtpServerPort  = P;
                m_RtcpServerPort = P+1;
                break;
            }
            else
            {
                udpsocketclose(m_RtpSocket);
                udpsocketclose(m_RtcpSocket);
            };
        }
    };
    ++m_udpRefCount;

    log_d("RTP Streamer set up with client IP %s and client Port %i", inet_ntoa(m_ClientIP), m_ClientPort);

    return true;
}

void AudioStreamer::ReleaseUdpTransport(void)
{
    --m_udpRefCount;
    if (m_udpRefCount == 0)
    {
        m_RtpServerPort  = 0;
        m_RtcpServerPort = 0;
        udpsocketclose(m_RtpSocket);
        udpsocketclose(m_RtcpSocket);

        m_RtpSocket = NULLSOCKET;
        m_RtcpSocket = NULLSOCKET;
    }
}

/*
int AudioStreamer::AddToStream(SAMPLE_TYPE * data, int len) {
    for (int i=0; i < len; i++) {
        if (xQueueSend(m_streamingData, &data[i], 0) != pdTRUE) {
            return i;
        }
    }

    return 0;
}
*/

void AudioStreamer::Start() {
    log_i("Starting RTP Stream");
    if (m_audioSource != NULL) {
        m_audioSource->start();
        esp_timer_start_periodic(RTP_timer, 20000);
    } else {
        log_e("No streaming source");
    }
}

void AudioStreamer::Stop() {
    log_i("Stopping RTP Stream");
    if (m_audioSource != NULL) {
        m_audioSource->stop();
    }
    esp_timer_stop(RTP_timer);
}

void AudioStreamer::doRTPStream(void * audioStreamerObj) {
    AudioStreamer * streamer = (AudioStreamer*)audioStreamerObj;
    int samples;
    int start, stop;

    start = esp_timer_get_time();

    samples = streamer->SendRtpPacketDirect();
    if (samples < 0) {
        log_w("Direct sending of RTP stream failed");
    } else if (samples > 0) {           // samples have been sent
        streamer->m_Timestamp += samples;        // no of samples sent
        log_v("%i samples sent (%ims); timestamp: %i", samples, samples / 16, streamer->m_Timestamp);
    }

    stop = esp_timer_get_time();
    if (stop - start > 20000) {
        log_w("RTP Stream can't keep up (took %i us, 20000 is max)!", stop - start);
    }
        
}