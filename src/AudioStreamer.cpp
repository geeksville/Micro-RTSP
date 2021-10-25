#include "AudioStreamer.h"
#include <stdio.h>

AudioStreamer::AudioStreamer()
{
    printf("Creating Audio streamer\n");
    m_RtpServerPort  = 0;
    m_RtcpServerPort = 0;

    m_SequenceNumber = 0;
    m_Timestamp      = 0;
    m_SendIdx        = 0;

    m_RtpSocket = NULLSOCKET;
    m_RtcpSocket = NULLSOCKET;

    m_prevMsec = 0;

    m_udpRefCount = 0;
};

AudioStreamer::~AudioStreamer()
{
    
};

int AudioStreamer::SendRtpPacket(unsigned const char* data, int len)
{
    // printf("CStreamer::SendRtpPacket offset:%d - begin\n", fragmentOffset);
#define KRtpHeaderSize 12           // size of the RTP header

#define MAX_FRAGMENT_SIZE 1100 // FIXME, pick more carefully

    static char RtpBuf[2048]; // Note: we assume single threaded, this large buf we keep off of the tiny stack
    if (len > MAX_FRAGMENT_SIZE) len = MAX_FRAGMENT_SIZE;

    int RtpPacketSize = len + KRtpHeaderSize;

    memset(RtpBuf,0x00,sizeof(RtpBuf));
    // Prepare the first 4 byte of the packet. This is the Rtp over Rtsp header in case of TCP based transport
    RtpBuf[0]  = '$';        // magic number
    RtpBuf[1]  = 0;          // number of multiplexed subchannel on RTPS connection - here the RTP channel
    RtpBuf[2]  = (RtpPacketSize & 0x0000FF00) >> 8;
    RtpBuf[3]  = (RtpPacketSize & 0x000000FF);
    // Prepare the 12 byte RTP header
    RtpBuf[4]  = 0x80;                               // RTP version
    RtpBuf[5]  = 0x0b;                               // L16 payload (11) and no marker bit
    RtpBuf[7]  = m_SequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
    RtpBuf[6]  = m_SequenceNumber >> 8;
    RtpBuf[8]  = (m_Timestamp & 0xFF000000) >> 24;   // each image gets a timestamp
    RtpBuf[9]  = (m_Timestamp & 0x00FF0000) >> 16;
    RtpBuf[10] = (m_Timestamp & 0x0000FF00) >> 8;
    RtpBuf[11] = (m_Timestamp & 0x000000FF);
    RtpBuf[12] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
    RtpBuf[13] = 0xf9;                               // we just an arbitrary number here to keep it simple
    RtpBuf[14] = 0x7e;
    RtpBuf[15] = 0x67;

    int headerLen = 16; 

    // append the JPEG scan data to the RTP buffer
    memcpy(RtpBuf + headerLen, data, len);

    m_SequenceNumber++;                              // prepare the packet counter for the next packet

    //IPADDRESS otherip;
    //IPPORT otherport;

    // RTP marker bit must be set on last fragment
    //if (m_Client->m_streaming && !session->m_stopped) {
        // UDP - we send just the buffer by skipping the 4 byte RTP over RTSP header
        //socketpeeraddr(session->getClient(), &otherip, &otherport);
    udpsocketsend(m_RtpSocket,&RtpBuf[0],RtpPacketSize, m_ClientIP, m_ClientPort);
    //}

    // printf("CStreamer::SendRtpPacket offset:%d - end\n", fragmentOffset);
    return len;
};

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

    printf("RTP Streamer set up with server Port %i and client Port %i\n", m_RtpServerPort, aClientPort);

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

