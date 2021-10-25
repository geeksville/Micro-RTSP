#pragma once

#include "platglue.h"
#include "IAudioSource.h"
#include <esp_timer.h>


class AudioStreamer
{
public:
    AudioStreamer();
    AudioStreamer(IAudioSource * source);
    virtual ~AudioStreamer();

    u_short GetRtpServerPort();
    u_short GetRtcpServerPort();

    bool InitUdpTransport(IPADDRESS aClientIP, IPPORT aClientPort);
    void ReleaseUdpTransport(void);

    //int AddToStream(SAMPLE_TYPE * data, int len);
    //int SendRtpPacket(SAMPLE_TYPE * data, int len);

    int SendRtpPacketDirect();

    int getSampleRate();

    void Start();

    void Stop();

    TaskHandle_t getTaskHandle() { return m_RTPTask; };

private:
    static void doRTPStream(void * audioStreamerObj);

    const int STREAMING_BUFFER_SIZE = 2048;
    unsigned char * RtpBuf;

    IAudioSource * m_audioSource = NULL;
    int m_samplingRate = 16000;
    int m_sampleSizeBytes = 2;
    int m_fragmentSize;
    int m_fragmentSizeBytes;
    const int HEADER_SIZE = 12;           // size of the RTP header

    //QueueHandle_t m_streamingData;

    TaskHandle_t m_RTPTask;

    UDPSOCKET m_RtpSocket;           // RTP socket for streaming RTP packets to client
    UDPSOCKET m_RtcpSocket;          // RTCP socket for sending/receiving RTCP packages

    IPPORT m_RtpServerPort;      // RTP sender port on server
    IPPORT m_RtcpServerPort;     // RTCP sender port on server

    u_short m_SequenceNumber;
    uint32_t m_Timestamp;
    int m_SendIdx;

    IPADDRESS m_ClientIP;
    IPPORT m_ClientPort;
    uint32_t m_prevMsec;

    int m_udpRefCount;

    esp_timer_handle_t RTP_timer;
};

