#include <Arduino.h>

#include "RTSPServer.h"
#include "AudioStreamer.h"
#include "AudioTestSource.h"
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


int main()
{
    connectToWiFi("unknown", "abec4007");

    AudioTestSource testSource = AudioTestSource();

    AudioStreamer<int16_t> streamer = AudioStreamer<int16_t>(&testSource);
    RTSPServer rtsp = RTSPServer(&streamer);

    rtsp.runAsync();
    
    while(1);

    log_e("Main is now returning!");
    return 0;
}

void setup() {
    log_d("Setup function, starting main");
    main();
}

void loop() {

}