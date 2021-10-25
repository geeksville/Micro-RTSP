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

RTSPServer * rtsp;

void setup() {
    connectToWiFi("unknown", "abec4007");

    AudioTestSource testSource = AudioTestSource();

    AudioStreamer streamer = AudioStreamer(&testSource);
    rtsp = new RTSPServer(&streamer);

    rtsp->runAsync();
    
    log_i("Set up done");
}

void loop() {
    vTaskDelay(500 / portTICK_PERIOD_MS);
    log_i("Free heap size: %i KB", esp_get_free_heap_size() / 1000);
    log_i("%s: Stack high watermark: %i KB",     
        pcTaskGetTaskName(NULL),     
        uxTaskGetStackHighWaterMark(NULL) / 1000     
    );
    log_i("%s: Stack high watermark: %i KB",     
        pcTaskGetTaskName(NULL),     
        uxTaskGetStackHighWaterMark(NULL) / 1000     
    );
}