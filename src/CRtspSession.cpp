#include "CRtspSession.h"
#include <stdio.h>
#include <ctime>

CRtspSession::CRtspSession(WiFiClient& aClient, AudioStreamer* aStreamer) :
 m_Client(aClient),
 m_Streamer(aStreamer)
{
    Init();

    // create buffers in heap
    RecvBuf = new char[RTSP_BUFFER_SIZE];
    CurRequest = new char[RTSP_BUFFER_SIZE];

    m_RtspClient = &m_Client;
    m_RtspSessionID  = getRandom();         // create a session ID
    //m_RtspSessionID |= 0x80000000;
    m_StreamID       = -1;
    m_ClientRTPPort  =  0;
    m_ClientRTCPPort =  0;
    m_streaming = false;
    m_stopped = false;

    m_RtpClientPort  = 0;
    m_RtcpClientPort = 0;
    log_i("RTSP session created");
};

CRtspSession::~CRtspSession()
{
    //m_Streamer->ReleaseUdpTransport();
    closesocket(m_RtspClient);
    
    delete RecvBuf;
    delete CurRequest;
};

void CRtspSession::Init()
{
    m_RtspCmdType   = RTSP_UNKNOWN;
    memset(m_URLPreSuffix, 0x00, sizeof(m_URLPreSuffix));
    memset(m_URLSuffix,    0x00, sizeof(m_URLSuffix));
    memset(m_CSeq,         0x00, sizeof(m_CSeq));
    memset(m_URLHostPort,  0x00, sizeof(m_URLHostPort));
    m_ContentLength  =  0;
};

bool CRtspSession::ParseRtspRequest(char const * aRequest, unsigned aRequestSize)
{
    unsigned CurRequestSize;

    log_v("aRequest: ------------------------\n%s\n-------------------------", aRequest);

    Init();
    CurRequestSize = aRequestSize;
    memcpy(CurRequest,aRequest,aRequestSize);

    // check whether the request contains information about the RTP/RTCP UDP client ports (SETUP command)
    char * ClientPortPtr;
    char * TmpPtr;
    static char CP[1024];
    char * pCP;

    ClientPortPtr = strstr(CurRequest,"client_port");
    if (ClientPortPtr != nullptr)
    {
        TmpPtr = strstr(ClientPortPtr,"\r\n");
        if (TmpPtr != nullptr)
        {
            TmpPtr[0] = 0x00;
            strcpy(CP,ClientPortPtr);
            pCP = strstr(CP,"=");
            if (pCP != nullptr)
            {
                pCP++;
                strcpy(CP,pCP);
                pCP = strstr(CP,"-");
                if (pCP != nullptr)
                {
                    pCP[0] = 0x00;
                    m_ClientRTPPort  = atoi(CP);
                    m_ClientRTCPPort = m_ClientRTPPort + 1;
                };
            };
        };
    };

    // Read everything up to the first space as the command name
    bool parseSucceeded = false;
    unsigned i;
    for (i = 0; i < sizeof(CmdName)-1 && i < CurRequestSize; ++i)
    {
        char c = CurRequest[i];
        if (c == ' ' || c == '\t')
        {
            parseSucceeded = true;
            break;
        }
        CmdName[i] = c;
    }
    CmdName[i] = '\0';
    if (!parseSucceeded) {
        log_e("failed to parse RTSP");
        return false;
    }

    log_i("RTSP received %s", CmdName);

    // find out the command type
    if (strstr(CmdName,"OPTIONS")   != nullptr) m_RtspCmdType = RTSP_OPTIONS; else
    if (strstr(CmdName,"DESCRIBE")  != nullptr) m_RtspCmdType = RTSP_DESCRIBE; else
    if (strstr(CmdName,"SETUP")     != nullptr) m_RtspCmdType = RTSP_SETUP; else
    if (strstr(CmdName,"PLAY")      != nullptr) m_RtspCmdType = RTSP_PLAY; else
    if (strstr(CmdName,"TEARDOWN")  != nullptr) m_RtspCmdType = RTSP_TEARDOWN; else 
    log_e("Error: Unsupported Command received (%s)!", CmdName);

    // Skip over the prefix of any "rtsp://" or "rtsp:/" URL that follows:
    unsigned j = i+1;
    while (j < CurRequestSize && (CurRequest[j] == ' ' || CurRequest[j] == '\t')) ++j; // skip over any additional white space
    for (; (int)j < (int)(CurRequestSize-8); ++j)
    {
        if ((CurRequest[j]   == 'r' || CurRequest[j]   == 'R')   &&
            (CurRequest[j+1] == 't' || CurRequest[j+1] == 'T') &&
            (CurRequest[j+2] == 's' || CurRequest[j+2] == 'S') &&
            (CurRequest[j+3] == 'p' || CurRequest[j+3] == 'P') &&
            CurRequest[j+4] == ':' && CurRequest[j+5] == '/')
        {
            j += 6;
            if (CurRequest[j] == '/')
            {   // This is a "rtsp://" URL; skip over the host:port part that follows:
                ++j;
                unsigned uidx = 0;
                while (j < CurRequestSize && CurRequest[j] != '/' && CurRequest[j] != ' ' && uidx < sizeof(m_URLHostPort) - 1)
                {   // extract the host:port part of the URL here
                    m_URLHostPort[uidx] = CurRequest[j];
                    uidx++;
                    ++j;
                };
            }
            else --j;
            i = j;
            break;
        }
    }

    log_v("m_URLHostPort: %s", m_URLHostPort);

    // Look for the URL suffix (before the following "RTSP/"):
    parseSucceeded = false;
    for (unsigned k = i+1; (int)k < (int)(CurRequestSize-5); ++k)
    {
        if (CurRequest[k]   == 'R'   && CurRequest[k+1] == 'T'   &&
            CurRequest[k+2] == 'S'   && CurRequest[k+3] == 'P'   &&
            CurRequest[k+4] == '/')
        {
            while (--k >= i && CurRequest[k] == ' ') {}
            unsigned k1 = k;
            while (k1 > i && CurRequest[k1] != '=') --k1;
            if (k - k1 + 1 > sizeof(m_URLSuffix)) {
                parseSucceeded = false;
            } else {
                unsigned n = 0, k2 = k1+1;

                while (k2 <= k) m_URLSuffix[n++] = CurRequest[k2++];
                m_URLSuffix[n] = '\0';

                if (k1 - i > sizeof(m_URLPreSuffix)) {
                    parseSucceeded = false; 
                } else {
                    parseSucceeded = true;
                }

                n = 0; k2 = i + 1;
                while (k2 <= k1 - 1) m_URLPreSuffix[n++] = CurRequest[k2++];
                m_URLPreSuffix[n] = '\0';
                i = k + 7;
            }
            
            break;
        }
    }
    log_v("m_URLSuffix: %s", m_URLSuffix);
    log_v("m_URLPreSuffix: %s", m_URLPreSuffix);
    log_v("URL Suffix parse succeeded: %i", parseSucceeded);

    // Look for "CSeq:", skip whitespace, then read everything up to the next \r or \n as 'CSeq':
    parseSucceeded = false;
    for (j = i; (int)j < (int)(CurRequestSize-5); ++j)
    {
        if (CurRequest[j]   == 'C' && CurRequest[j+1] == 'S' &&
            CurRequest[j+2] == 'e' && CurRequest[j+3] == 'q' &&
            CurRequest[j+4] == ':')
        {
            j += 5;
            while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
            unsigned n;
            for (n = 0; n < sizeof(m_CSeq)-1 && j < CurRequestSize; ++n,++j)
            {
                char c = CurRequest[j];
                if (c == '\r' || c == '\n')
                {
                    parseSucceeded = true;
                    break;
                }
                m_CSeq[n] = c;
            }
            m_CSeq[n] = '\0';
            break;
        }
    }
    log_v("Look for CSeq success: %i", parseSucceeded);
    if (!parseSucceeded) return false;

    // Also: Look for "Content-Length:" (optional)
    for (j = i; (int)j < (int)(CurRequestSize-15); ++j)
    {
        if (CurRequest[j]    == 'C'  && CurRequest[j+1]  == 'o'  &&
            CurRequest[j+2]  == 'n'  && CurRequest[j+3]  == 't'  &&
            CurRequest[j+4]  == 'e'  && CurRequest[j+5]  == 'n'  &&
            CurRequest[j+6]  == 't'  && CurRequest[j+7]  == '-'  &&
            (CurRequest[j+8] == 'L' || CurRequest[j+8]   == 'l') &&
            CurRequest[j+9]  == 'e'  && CurRequest[j+10] == 'n' &&
            CurRequest[j+11] == 'g' && CurRequest[j+12]  == 't' &&
            CurRequest[j+13] == 'h' && CurRequest[j+14] == ':')
        {
            j += 15;
            while (j < CurRequestSize && (CurRequest[j] ==  ' ' || CurRequest[j] == '\t')) ++j;
            unsigned num;
            if (sscanf(&CurRequest[j], "%u", &num) == 1) m_ContentLength = num;
        }
    }
    return true;
};

RTSP_CMD_TYPES CRtspSession::Handle_RtspRequest(char const * aRequest, unsigned aRequestSize)
{
    if (ParseRtspRequest(aRequest,aRequestSize))
    {
        switch (m_RtspCmdType)
        {
        case RTSP_OPTIONS:  { Handle_RtspOPTION();   break; };
        case RTSP_DESCRIBE: { Handle_RtspDESCRIBE(); break; };
        case RTSP_SETUP:    { Handle_RtspSETUP();    break; };
        case RTSP_PLAY:     { Handle_RtspPLAY();     break; };
        case RTSP_TEARDOWN: { Handle_RtspTEARDOWN(); break; };
        default: {};
        };
    };
    return m_RtspCmdType;
};

void CRtspSession::Handle_RtspOPTION()
{
    static char Response[1024]; // Note: we assume single threaded, this large buf we keep off of the tiny stack

    snprintf(Response,sizeof(Response),
             "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
             "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n",m_CSeq);

    socketsend(m_RtspClient,Response,strlen(Response));
}

void CRtspSession::Handle_RtspDESCRIBE()
{
    static char Response[1024]; // Note: we assume single threaded, this large buf we keep off of the tiny stack
    static char SDPBuf[1024];
    static char URLBuf[1024];

    // check whether we know a stream with the URL which is requested
    m_StreamID = -1;        // invalid URL
    // find Stream ID
    if ( strcmp(m_URLPreSuffix, STD_URL_PRE_SUFFIX) == 0) {
        char* end;
        m_StreamID = strtol(m_URLSuffix, &end, 10);
        if (*end != '\0') m_StreamID = -1;
    }

    // simulate DESCRIBE server response
    static char OBuf[256];
    char * ColonPtr;
    strcpy(OBuf,m_URLHostPort);
    ColonPtr = strstr(OBuf,":");
    if (ColonPtr != nullptr) ColonPtr[0] = 0x00;

    snprintf(SDPBuf,sizeof(SDPBuf),
             "v=0\r\n"                              //SDP Version
             "o=- %d 0 IN IP4 %s\r\n"
             "s=Microphone\r\n"                     // Stream Name
             "c=IN IP4 0.0.0.0\r\n"                 // Connection Information
             "t=0 0\r\n"                            // start / stop - 0 -> unbounded and permanent session
             "m=audio 0 RTP/AVP 11\r\n"             // currently we just handle UDP sessions
             // Media Attributes
             "a=rtpmap:11 L16/%i/1\r\n"
             "a=rate:%i\r\n"
             "a=control:%s=0"
             //"a=ftmp"

             ,
             rand() & 0xFF,
             OBuf, 
             m_Streamer->getSampleRate(),
             m_Streamer->getSampleRate(),
             STD_URL_PRE_SUFFIX);

    snprintf(URLBuf,sizeof(URLBuf),
             "rtsp://%s",
             m_URLHostPort);
    snprintf(Response,sizeof(Response),
             "RTSP/1.0 200 OK\r\nCSeq: %s\r\n"
             "%s\r\n"
             "Content-Base: %s/\r\n"
             "Content-Type: application/sdp\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s",
             m_CSeq,
             DateHeader(),
             URLBuf,
             (int) strlen(SDPBuf),
             SDPBuf);

    socketsend(m_RtspClient,Response,strlen(Response));
}

void CRtspSession::InitTransport(u_short aRtpPort, u_short aRtcpPort)
{
    m_RtpClientPort  = aRtpPort;
    m_RtcpClientPort = aRtcpPort;

    IPADDRESS clientIP;
    IPPORT clientPort;
    socketpeeraddr(m_RtspClient, &clientIP, &clientPort);

    m_Streamer->InitUdpTransport(clientIP, m_RtpClientPort);
};

void CRtspSession::Handle_RtspSETUP()
{
    static char Response[1024];
    static char Transport[255];

    // init RTSP Session transport type (UDP or TCP) and ports for UDP transport
    InitTransport(m_ClientRTPPort,m_ClientRTCPPort);

    // simulate SETUP server response
    snprintf(Transport,sizeof(Transport),
                 "RTP/AVP;unicast;destination=127.0.0.1;source=127.0.0.1;client_port=%i-%i;server_port=%i-%i",
                 //"RTP/AVP;unicast;client_port=%i-%i;server_port=%i-%i",
                 m_ClientRTPPort,
                 m_ClientRTCPPort,
                 m_Streamer->GetRtpServerPort(),
                 m_Streamer->GetRtcpServerPort());
    snprintf(Response,sizeof(Response),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %s\r\n"
             "%s\r\n"
             "Session: %i\r\n"
             "Transport: %s\r\n"
             "\r\n",
             m_CSeq,
             DateHeader(),
             m_RtspSessionID,
             Transport
             
             );

    socketsend(m_RtspClient,Response,strlen(Response));
}

void CRtspSession::Handle_RtspPLAY()
{
    static char Response[1024];

    // simulate SETUP server response
    snprintf(Response,sizeof(Response),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %s\r\n"
             //"%s\r\n"
             "Range: npt=0.000-\r\n"                // this is necessary
             "Session: %i\r\n\r\n"
             //"RTP-Info: url=rtsp://127.0.0.1:8554/%s=0\r\n\r\n",          // TODO whats this
             ,
             m_CSeq,
             //DateHeader(),
             m_RtspSessionID
             //STD_URL_PRE_SUFFIX
             );

    socketsend(m_RtspClient,Response,strlen(Response));

    m_Streamer->Start();
}

void CRtspSession::Handle_RtspTEARDOWN()
{
    static char Response[1024];

    m_Streamer->Stop();

    // simulate SETUP server response
    snprintf(Response,sizeof(Response),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %s\r\n\r\n",
             m_CSeq);

    socketsend(m_RtspClient,Response,strlen(Response));

    m_sessionOpen = false;
}

char const * CRtspSession::DateHeader()
{
    static char buf[200];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT", gmtime(&tt));
    return buf;
}

int CRtspSession::GetStreamID()
{
    return m_StreamID;
};



/**
   Read from our socket, parsing commands as possible.
 */
bool CRtspSession::handleRequests(uint32_t readTimeoutMs)
{
    if(m_stopped)
        return false; // Already closed down

    memset(RecvBuf,0x00, RTSP_BUFFER_SIZE);
    int res = socketread(m_RtspClient, RecvBuf, RTSP_BUFFER_SIZE, readTimeoutMs);
    if(res > 0) {
        // we filter away everything which seems not to be an RTSP command: O-ption, D-escribe, S-etup, P-lay, T-eardown
        if ((RecvBuf[0] == 'O') || (RecvBuf[0] == 'D') || (RecvBuf[0] == 'S') || (RecvBuf[0] == 'P') || (RecvBuf[0] == 'T'))
        {
            RTSP_CMD_TYPES C = Handle_RtspRequest(RecvBuf,res);
            // TODO this should go in the handling functions
            if (C == RTSP_PLAY)
                m_streaming = true;
            else if (C == RTSP_TEARDOWN) {
                m_stopped = true;
            }
                
        }
        return true;
    }
    else if(res == 0) {
        log_w("client closed socket, exiting");
        m_stopped = true;
        return true;
    }
    else  {
        // Timeout on read
        //printf("RTSP read timeout\n");
        return false;
    }
}
