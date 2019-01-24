#include "platglue.h"

#include "CStreamer.h"
#include "CRtspSession.h"
#include "JPEGSamples.h"

#include <assert.h>

typedef unsigned char *BufPtr;

// When JPEG is stored as a file it is wrapped in a container
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
unsigned decodeJPEGfile(BufPtr *start) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    unsigned char *bytes = *start;

    assert(bytes[0] == 0xff);
    assert(bytes[1] == 0xd8);

    // kinda skanky, will break if unlucky and the headers inxlucde 0xffda
    // might fall off array if jpeg is invalid
    while(true) {
        while(*bytes++ != 0xff)
            ;
        if(*bytes++ == 0xda) {
            unsigned skipped = bytes - *start;
            printf("first byte %x, skipped %d\n", *bytes, skipped);

            *start = bytes;
            return skipped;
        }
    }

    // if we have to properly parse headers
    // unsigned len = bytes[2] * 256 + bytes[3] - 2;
    // *start = bytes + 4;
}

void workerThread(SOCKET Client)
{
    char RecvBuf[10000];                            // receiver buffer
    bool showBig = true;
    CStreamer Streamer(Client, showBig ? 640 : 64, showBig ? 480 : 48);                     // our streamer for UDP/TCP based RTP transport

    CRtspSession RtspSession(Client,&Streamer);     // our threads RTSP session and state
    int StreamID = 0;                               // the ID of the 2 JPEG samples streams which we support
    int frameoffset = 0;

    int msecsPerFrame = 100;

    // Use a timeout on our socket read to instead serve frames
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = msecsPerFrame * 1000; // send a new frame ever
    setsockopt(Client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);

    bool StreamingStarted = false;
    bool Stop             = false;

    while (!Stop)
    {
        memset(RecvBuf,0x00,sizeof(RecvBuf));
        int res = recv(Client,RecvBuf,sizeof(RecvBuf),0);
        if(res > 0) {
            // we filter away everything which seems not to be an RTSP command: O-ption, D-escribe, S-etup, P-lay, T-eardown
            if ((RecvBuf[0] == 'O') || (RecvBuf[0] == 'D') || (RecvBuf[0] == 'S') || (RecvBuf[0] == 'P') || (RecvBuf[0] == 'T'))
            {
                RTSP_CMD_TYPES C = RtspSession.Handle_RtspRequest(RecvBuf,res);
                if (C == RTSP_PLAY)
                    StreamingStarted = true;
                else if (C == RTSP_TEARDOWN)
                    Stop = true;
            }
        }
        else if(res == 0) {
            printf("client closed socket, exiting\n");
            Stop = true;
        }
        else if(res < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // Timeout on read

                // Send a frame
                if (StreamingStarted) {
                    printf("serving a frame\n");

                    if(showBig) {
                        BufPtr bytes = octo_jpg;
                        unsigned skipped = decodeJPEGfile(&bytes);
                        Streamer.StreamImage(bytes, octo_jpg_len - skipped);
                    }
                    else {
                        unsigned char  * Samples2[2] = { JpegScanDataCh2A, JpegScanDataCh2B };

                        Streamer.StreamImage(Samples2[frameoffset], KJpegCh2ScanDataLen);
                        frameoffset = (frameoffset + 1) % 2;
                    }
                }
            }
            else {
                // Unexpected error
                printf("Unexpected error %d, closing stream\n", res);
                Stop = true;
            }
        }
    }
    printf("Closing connection\n");
    closesocket(Client);
};

int main()
{
    SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
    SOCKET ClientSocket;                                      // RTSP socket to handle an client
    sockaddr_in ServerAddr;                                   // server address parameters
    sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
    socklen_t ClientAddrLen = sizeof(ClientAddr);

    printf("running RTSP server\n");

    ServerAddr.sin_family      = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    ServerAddr.sin_port        = htons(8554);                 // listen on RTSP port 8554
    MasterSocket               = socket(AF_INET,SOCK_STREAM,0);

    int enable = 1;
    if (setsockopt(MasterSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed");
        return 0;
    }

    // bind our master socket to the RTSP port and listen for a client connection
    if (bind(MasterSocket,(sockaddr*)&ServerAddr,sizeof(ServerAddr)) != 0) {
        printf("error can't bind port\n");

        return 0;
    }
    if (listen(MasterSocket,5) != 0) return 0;

    while (true)
    {   // loop forever to accept client connections
        ClientSocket = accept(MasterSocket,(struct sockaddr*)&ClientAddr,&ClientAddrLen);
        printf("Client connected. Client address: %s\r\n",inet_ntoa(ClientAddr.sin_addr));
        if(fork() == 0)
            workerThread(ClientSocket);
    }

    closesocket(MasterSocket);

    return 0;
}
