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

#include "AudioStreamer.h"
#include "platglue.h"

/// Supported RTSP command types
enum RTSP_CMD_TYPES
{
    RTSP_OPTIONS,
    RTSP_DESCRIBE,
    RTSP_SETUP,
    RTSP_PLAY,
    RTSP_TEARDOWN,
    RTSP_UNKNOWN
};

/// Buffer size for incoming requests, and outgoing responses
#define RTSP_BUFFER_SIZE       10000
/// Size of RTSP parameter buffers
#define RTSP_PARAM_STRING_MAX  100
/// Buffer size for RTSP host name
#define MAX_HOSTNAME_LEN       256

/**
 * Class for handling an RTSP session
 * Requires a an AudioStreamer as a source for Streaming
 */
class RtspSession 
{
private:
    const char * STD_URL_PRE_SUFFIX = "trackID";

    // global session state parameters
    int m_RtspSessionID;
    WiFiClient m_Client;
    SOCKET m_RtspClient;                                      // RTSP socket of that session
    int m_StreamID;                                           // number of simulated stream of that session
    IPPORT m_ClientRTPPort;                                  // client port for UDP based RTP transport
    IPPORT m_ClientRTCPPort;                                 // client port for UDP based RTCP transport
    AudioStreamer * m_Streamer;                                // the UDP streamer of that session

    // parameters of the last received RTSP request
    RTSP_CMD_TYPES m_RtspCmdType;                             // command type (if any) of the current request
    char m_URLPreSuffix[RTSP_PARAM_STRING_MAX];               // stream name pre suffix
    char m_URLSuffix[RTSP_PARAM_STRING_MAX];                  // stream name suffix
    char m_CSeq[RTSP_PARAM_STRING_MAX];                       // RTSP command sequence number
    char m_URLHostPort[MAX_HOSTNAME_LEN];                     // host:port part of the URL
    unsigned m_ContentLength;                                 // SDP string size

    uint16_t m_RtpClientPort;      // RTP receiver port on client (in host byte order!)
    uint16_t m_RtcpClientPort;     // RTCP receiver port on client (in host byte order!)
public:
    bool m_streaming;
    bool m_stopped;
    bool m_sessionOpen = true;
    char * RecvBuf;
    char * CurRequest;
    char CmdName[RTSP_PARAM_STRING_MAX];

public:
    /**
     * Creates a new RTSP session object
     * @param aRtspClient socket to which the RTSP client has connected
     * @param aStreamer AudioStreamer to be used a source for an RTP stream
     */
    RtspSession(WiFiClient& aRtspClient, AudioStreamer* aStreamer);

    /**
     * Closes socket and deletes allocated memory
     */
    ~RtspSession();

    /**
     * Read from the socket and parse commands if possible
     * @param readTimeoutMs time in milliseconds to wait for incoming data
     * @return false if the read timed out
     */
    bool handleRequests(uint32_t readTimeoutMs);

private:
    /**
     * Initializes memory
     */
    void Init();

    /**
     * Parses the an RTSP request and calls the response function depending on the type of command
     * @param aRequest c string containing the request
     * @param aRequestSize length of the request
     * @return the command type of the RTSP request
     */
    RTSP_CMD_TYPES Handle_RtspRequest(char const * aRequest, unsigned aRequestSize);

    /**
     * Parses an RTSP request, storing the extracted information in the RTSPSession object
     * @param aRequest c string containing the request
     * @param aRequestSize length of the request
     * @return true if parsing was successful
     */
    bool ParseRtspRequest(char const * aRequest, unsigned aRequestSize);

    /**
     * Sends Response to OPTIONS command
     */
    void Handle_RtspOPTION();

    /**
     * Sends Response to DESCRIBE command
     */
    void Handle_RtspDESCRIBE();

    /**
     * Sends Response to SETUP command and prepares RTP stream
     */
    void Handle_RtspSETUP();

    /**
     * Sends Response to PLAY command and starts the RTP stream
     */
    void Handle_RtspPLAY();

    /**
     * Sends Response to TEARDOWN command, stops the RTP stream
     */
    void Handle_RtspTEARDOWN();

    /**
     * Gives the current stream ID
     * @return ID of current stream or -1 if invalid
     */
    int GetStreamID();

    /**
     * Prepares sockets for RTP stream
     * @param aRtpPort local port number for RTP connection
     * @param aRtcpPort local port number for RTCP connection
     */
    void InitTransport(u_short aRtpPort, u_short aRtcpPort);

    SOCKET& getClient() { return m_RtspClient; }
    
    uint16_t getRtpClientPort() { return m_RtpClientPort; }

    /**
     * Create the DateHeader string for RTSP responses
     * @return pointer to Date Header string
     */
    char const * DateHeader();

};
