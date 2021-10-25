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

/**
 * Creates an RTSP Server to listen for client connections and start sessions
 */
class RTSPServer {
    private:
        TaskHandle_t serverTaskHandle;
        TaskHandle_t sessionTaskHandle;
        SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
        SOCKET ClientSocket;                                      // RTSP socket to handle an client
        sockaddr_in ServerAddr;                                   // server address parameters
        sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
        int port;               // port that the RTSP Server listens on
        int core;               // the ESP32 core number the RTSP runs on (0 or 1)
    
        int numClients = 0;         // number of connected clients

        AudioStreamer * streamer;   // AudioStreamer object that acts as a source for data streams

    public:
        /**
         * Creates a new RTSP server
         * @param streamer AudioStreamer object that acts as a source for data streams
         * @param port port that the RTSP Server should listen on (default 8554)
         * @param core the ESP32 core number the RTSP runs on (0 or 1, default 1)
         */
        RTSPServer(AudioStreamer * streamer, int port = 8554, int core = 1);

        /**
         * Starts running the server in a new asynchronous Task
         * @return 0 on success, or error number
         */
        int runAsync();

        TaskHandle_t getTaskHandle() { return serverTaskHandle; };

    private:
        /**
         * Routine for main server thread, listens for new clients 
         */ 
        static void serverThread(void* server_obj);

        /**
         * Routine for a session if it is started
         */
        static void sessionThread(void* server_obj);
};