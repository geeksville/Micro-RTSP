#include <Arduino.h>
#include "platglue.h"

#include "CRtspSession.h"
#include "AudioStreamer.h"
#include <assert.h>
#include <sys/time.h>
#include <WiFi.h>

bool connectToWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
  }
  log_i("Connected to %s", ssid);
  return true;
}


void workerThread(void * socket_obj) {
    SOCKET s = (SOCKET) socket_obj;
    AudioStreamer streamer = AudioStreamer();                     // our streamer for UDP/TCP based RTP transport
    bool frameSent = false;

    CRtspSession rtsp = CRtspSession(*s, &streamer);     // our threads RTSP session and state

    while (!rtsp.m_stopped && !frameSent)
    {
        uint32_t timeout = 400;
        if(!rtsp.handleRequests(timeout)) {
            printf("Request handling returned false\n");
            struct timeval now;
            gettimeofday(&now, NULL); // crufty msecish timer
            //uint32_t msec = now.tv_sec * 1000 + now.tv_usec / 1000;
            //rtsp.broadcastCurrentFrame(msec);
            frameSent = true;
            //printf("Audio File has been sent\n");
        } else {
            printf("Request handling successful\n");
        }

        if (rtsp.m_streaming) {
            // Stream RTP data

        }
    }

    printf("workerThread stopped\n");
    while(1);
}

int main()
{
    SOCKET MasterSocket;                                      // our masterSocket(socket that listens for RTSP client connections)
    SOCKET ClientSocket;                                      // RTSP socket to handle an client
    sockaddr_in ServerAddr;                                   // server address parameters
    sockaddr_in ClientAddr;                                   // address parameters of a new RTSP client
    socklen_t ClientAddrLen = sizeof(ClientAddr);
    TaskHandle_t workerHandle;

    connectToWiFi("unknown", "abec4007");

    printf("running RTSP server\n");

    ServerAddr.sin_family      = AF_INET;
    ServerAddr.sin_addr.s_addr = INADDR_ANY;
    ServerAddr.sin_port        = htons(8554);                 // listen on RTSP port 8554
    int s = socket(AF_INET,SOCK_STREAM,0);
    printf("Master socket fd: %i\n", s);
    MasterSocket               = new WiFiClient(s);
    if (MasterSocket == NULL) {
        printf("MasterSocket object couldnt be created\n");
        return 0;
    }

    printf("Master Socket created; fd: %i\n", MasterSocket->fd());

    int enable = 1;
    if (setsockopt(MasterSocket->fd(), SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        printf("setsockopt(SO_REUSEADDR) failed");
        return 0;
    }

    printf("Socket options set\n");

    // bind our master socket to the RTSP port and listen for a client connection
    if (bind(MasterSocket->fd(),(sockaddr*)&ServerAddr,sizeof(ServerAddr)) != 0) {
        printf("error can't bind port errno=%d\n", errno);

        return 0;
    }
    printf("Socket bound. Starting to listen\n");
    if (listen(MasterSocket->fd(),5) != 0) return 0;

    printf("Listening...\n");

    while (true)
    {   // loop forever to accept client connections
        ClientSocket = new WiFiClient(accept(MasterSocket->fd(),(struct sockaddr*)&ClientAddr,&ClientAddrLen));
        printf("Client connected. Client address: %s\r\n",inet_ntoa(ClientAddr.sin_addr));
        xTaskCreate(workerThread, "workerThread", 10000, (void*)ClientSocket, 0, &workerHandle);
        //if(fork() == 0)
        //    workerThread(ClientSocket);
    }

    closesocket(MasterSocket);

    return 0;
}

void setup() {
    main();
}

void loop() {
    log_i("In loop");

    while(1);
}