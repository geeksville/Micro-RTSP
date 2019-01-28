* add instructions for example app
* push RTSP streams to other servers ( https://github.com/ant-media/Ant-Media-Server/wiki/Getting-Started )
* make stack larger so that the various scratch buffers (currently in bss) can be shared
* cleanup code to a less ugly unified coding standard
* support multiple simultaneous clients on the device
* make octocat test image work again (by changing encoding type from 1 to 0 (422 vs 420))

DONE:
* serve real jpegs (use correct quantization & huffman tables)
* test that both TCP and UDP clients work
* change framerate to something slow
* test remote access
* select a licence and put license into github
* find cause of new mystery pause when starting up in sim mode
* split sim code from real code via inheritence
* use device camera
* package the ESP32-CAM stuff as a library so I can depend on it
* package as a library https://docs.platformio.org/en/latest/librarymanager/creating.html#library-creating-examples
