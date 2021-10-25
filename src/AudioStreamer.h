#pragma once

#include "platglue.h"


class AudioStreamer
{
public:
    AudioStreamer();
    virtual ~AudioStreamer();

    u_short GetRtpServerPort();
    u_short GetRtcpServerPort();

    bool InitUdpTransport(IPADDRESS aClientIP, IPPORT aClientPort);
    void ReleaseUdpTransport(void);

    void Stream(unsigned char * data, int len);

    int SendRtpPacket(unsigned const char* data, int len);

private:
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
};

