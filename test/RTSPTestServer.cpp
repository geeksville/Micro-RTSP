#include "platglue.h"

#include "CStreamer.h"
#include "CRtspSession.h"

#include <assert.h>


void workerThread(SOCKET Client)
{
    bool showBig = true;
    CStreamer Streamer(Client, showBig ? 640 : 64, showBig ? 480 : 48);                     // our streamer for UDP/TCP based RTP transport

    CRtspSession rtsp(Client,&Streamer);     // our threads RTSP session and state

    while (!rtsp.m_stopped)
    {
        rtsp.doIdle();
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
