#include "RTSPServer.h"
#include "CRtspSession.h"

RTSPServer::RTSPServer(AudioStreamer * streamer, int port) {
    log_i("RTSP Audio version: " RTSP_AUDIO_VERSION);
    this->streamer = streamer;
    this->port = port;
}

int RTSPServer::runAsync() {
    int error;

    printf("running RTSP server\n");

    ServerAddr.sin_family      = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    ServerAddr.sin_port        = htons(port);                 // listen on RTSP port 8554 as default
    int s = socket(AF_INET,SOCK_STREAM,0);
    printf("Master socket fd: %i\n", s);
    MasterSocket               = new WiFiClient(s);
    if (MasterSocket == NULL) {
        printf("MasterSocket object couldnt be created\n");
        return -1;
    }

    printf("Master Socket created; fd: %i\n", MasterSocket->fd());

    int enable = 1;
    error = setsockopt(MasterSocket->fd(), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (error < 0) {
        printf("setsockopt(SO_REUSEADDR) failed");
        return error;
    }

    printf("Socket options set\n");

    // bind our master socket to the RTSP port and listen for a client connection
    error = bind(MasterSocket->fd(),(sockaddr*)&ServerAddr,sizeof(ServerAddr));
    if (error != 0) {
        printf("error can't bind port errno=%d\n", errno);
        return error;
    }
    printf("Socket bound. Starting to listen\n");
    error = listen(MasterSocket->fd(),5);
    if (error != 0) {
        printf("Error while listening\n");
        return error;
    }

    if (xTaskCreate(RTSPServer::serverThread, "RTSPServerThread", 10000, (void*)this, 0, &workerHandle) != pdPASS) {
        printf("Couldn't create server thread");
        return -1;
    } 


    return 0;
}

void RTSPServer::serverThread(void* server_obj) {
    socklen_t ClientAddrLen = sizeof(ClientAddr);
    RTSPServer * server = (RTSPServer*) server_obj;
   
    printf("Server thread listening...\n");

    while (true)
    {   // loop forever to accept client connections
        // TODO individual sockets
        server->ClientSocket = new WiFiClient(accept(server->MasterSocket->fd(),(struct sockaddr*)&server->ClientAddr,&ClientAddrLen));
        printf("Client connected. Client address: %s\n",inet_ntoa(server->ClientAddr.sin_addr));
        if (xTaskCreate(RTSPServer::workerThread, "workerThread", 10000, (void*)server, 0, NULL) != pdPASS) {
            printf("Couldn't create workerThread\n");
        } else {
            printf("Created workerThread\n");
            server->numClients++;
        }
        //vTaskResume(workerHandle);
        // TODO only ONE task used repeatedly
    }

    // should never be reached
    closesocket(server->MasterSocket);
}


void RTSPServer::workerThread(void * server_obj) {
    RTSPServer * server = (RTSPServer*)server_obj;
    AudioStreamer * streamer = server->streamer;
    SOCKET s = server->ClientSocket;

        // stop this task - wait for a client to connect
        //vTaskSuspend(NULL);
        // TODO check if everything is ok to go
        printf("Client connected\n");

        CRtspSession rtsp = CRtspSession(*s, streamer);     // our threads RTSP session and state

        while (rtsp.m_sessionOpen)
        {
            uint32_t timeout = 400;
            if(!rtsp.handleRequests(timeout)) {
                printf("Request handling returned false\n");
                struct timeval now;
                gettimeofday(&now, NULL); // crufty msecish timer
                //uint32_t msec = now.tv_sec * 1000 + now.tv_usec / 1000;
                //rtsp.broadcastCurrentFrame(msec);
                //printf("Audio File has been sent\n");
            } else {
                printf("Request handling successful\n");
            }

            if (rtsp.m_streaming) {
                // Stream RTP data
                //streamer->Start();
            }
        }

    
    // should never be reached
    log_e("workerThread stopped, deleting task");

    vTaskDelete(NULL);
}