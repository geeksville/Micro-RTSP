#include "platglue.h"

#include "SimStreamer.h"
#include "CRtspSession.h"
#include "JPEGSamples.h"
#include <assert.h>

// From RFC2435 generates standard quantization tables

/*
 * Table K.1 from JPEG spec.
 */
static const int jpeg_luma_quantizer[64] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

/*
 * Table K.2 from JPEG spec.
 */
static const int jpeg_chroma_quantizer[64] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

/*
 * Call MakeTables with the Q factor and two u_char[64] return arrays
 */
void
MakeTables(int q, u_char *lqt, u_char *cqt)
{
    int i;
    int factor = q;

    if (q < 1) factor = 1;
    if (q > 99) factor = 99;
    if (q < 50)
        q = 5000 / factor;
    else
        q = 200 - factor*2;

    for (i=0; i < 64; i++) {
        int lq = (jpeg_luma_quantizer[i] * q + 50) / 100;
        int cq = (jpeg_chroma_quantizer[i] * q + 50) / 100;

        /* Limit the quantizers to 1 <= q <= 255 */
        if (lq < 1) lq = 1;
        else if (lq > 255) lq = 255;
        lqt[i] = lq;

        if (cq < 1) cq = 1;
        else if (cq > 255) cq = 255;
        cqt[i] = cq;
    }
}


// analyze an imge from our camera to find which quant table it is using...
// Used to see if our camera is spitting out standard RTP tables (it isn't)

// So we have to use Q of 255 to indicate that each frame has unique quant tables
// use 0 for precision in the qant header, 64 for length
void findCameraQuant()
{
    BufPtr bytes = capture_jpg;
    uint32_t len = capture_jpg_len;

    if(!findJPEGheader(&bytes, &len, 0xdb)) {
        printf("error can't find quant table 0\n");
        return;
    }
    else {
        printf("found quant table %x\n", bytes[2]);
    }
    BufPtr qtable0 = bytes + 3; // 3 bytes of header skipped

    if(!findJPEGheader(&bytes, &len, 0xdb)) {
        printf("error can't find quant table 1\n");
        return;
    }
    else {
        printf("found quant table %x\n", bytes[2]);
    }
    BufPtr qtable1 = bytes + 3;

    for(int q = 0; q < 128; q++) {
        uint8_t lqt[64], cqt[64];
        MakeTables(q, lqt, cqt);

        if(memcmp(qtable0, lqt, sizeof(lqt)) == 0 && memcmp(qtable1, cqt, sizeof(cqt)) == 0) {
            printf("Found matching quant table %d\n", q);
        }
    }
    printf("No matching quant table found!\n");
}

void workerThread(SOCKET s)
{
    SimStreamer streamer(s, true);                     // our streamer for UDP/TCP based RTP transport

    CRtspSession rtsp(s, &streamer);     // our threads RTSP session and state

    while (!rtsp.m_stopped)
    {
        uint32_t timeout = 100;
        if(!rtsp.handleRequests(timeout))
            rtsp.broadcastCurrentFrame();
    }
}

int main()
{
    SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
    SOCKET ClientSocket;                                      // RTSP socket to handle an client
    sockaddr_in ServerAddr;                                   // server address parameters
    sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
    socklen_t ClientAddrLen = sizeof(ClientAddr);

    printf("running RTSP server\n");
    findCameraQuant();

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
        printf("error can't bind port errno=%d\n", errno);

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
