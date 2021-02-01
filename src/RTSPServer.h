#pragma once

#include "AudioStreamer.h"

#define RTSP_AUDIO_VERSION     "1_0_0"

class RTSPServer {
    private:
        TaskHandle_t workerHandle;
        SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
        SOCKET ClientSocket;                                      // RTSP socket to handle an client
        sockaddr_in ServerAddr;                                   // server address parameters
        sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
        int port;
    
        int numClients = 0;

        AudioStreamer * streamer;

    public:
        RTSPServer(AudioStreamer * streamer, int port = 8554);
        int runAsync();

    private:
        static void serverThread(void* server_obj);
        static void workerThread(void* server_obj);
};