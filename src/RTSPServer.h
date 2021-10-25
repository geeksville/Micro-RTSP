#pragma once

#include "AudioStreamer.h"

class RTSPServer {
    private:
        TaskHandle_t workerHandle;
        SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
        SOCKET ClientSocket;                                      // RTSP socket to handle an client
        sockaddr_in ServerAddr;                                   // server address parameters
        sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
        int port;
    
        int numClients = 0;

        // TODO allow any types
        AudioStreamer<int16_t> * streamer;

    public:
        RTSPServer(AudioStreamer<int16_t> * streamer, int port = 8554);
        int runAsync();

    private:
        static void serverThread(void* server_obj);
        static void workerThread(void* server_obj);
};