#pragma once

#include "platglue.h"

class CStreamer
{
public:
    CStreamer(SOCKET aClient, u_short width, u_short height);
    ~CStreamer();

    void    InitTransport(u_short aRtpPort, u_short aRtcpPort, bool TCP);
    u_short GetRtpServerPort();
    u_short GetRtcpServerPort();
    void    StreamImage(unsigned char *data, int dataLen);

private:
    int    SendRtpPacket(unsigned char *jpeg, int jpegLen, int fragmentOffset);// returns new fragmentOffset or 0 if finished with frame

    SOCKET m_RtpSocket;           // RTP socket for streaming RTP packets to client
    SOCKET m_RtcpSocket;          // RTCP socket for sending/receiving RTCP packages

    u_short m_RtpClientPort;      // RTP receiver port on client
    u_short m_RtcpClientPort;     // RTCP receiver port on client
    u_short m_RtpServerPort;      // RTP sender port on server
    u_short m_RtcpServerPort;     // RTCP sender port on server

    u_short m_SequenceNumber;
    DWORD m_Timestamp;
    int m_SendIdx;
    bool m_TCPTransport;
    SOCKET m_Client;

    u_short m_width; // image data info
    u_short m_height;
};
