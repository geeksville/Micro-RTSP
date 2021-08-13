#pragma once

#include "AudioStreamer.h"

class RTSPServer {
    private:
        TaskHandle_t serverTaskHandle;
        TaskHandle_t sessionTaskHandle;
        SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
        SOCKET ClientSocket;                                      // RTSP socket to handle an client
        sockaddr_in ServerAddr;                                   // server address parameters
        sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
        int port;
        int core;
    
        int numClients = 0;

        // TODO allow any types
        AudioStreamer * streamer;

    public:
        RTSPServer(AudioStreamer * streamer, int port = 8554, int core = 1);
        int runAsync();
        TaskHandle_t getTaskHandle() { return serverTaskHandle; };

    private:
        static void serverThread(void* server_obj);
        static void sessionThread(void* server_obj);
};