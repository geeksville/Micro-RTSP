
#include "SimStreamer.h"
#include "JPEGSamples.h"


SimStreamer::SimStreamer(SOCKET aClient, bool showBig) : CStreamer(aClient, showBig ? 800 : 640, showBig ? 600 : 480)
{
    m_showBig = showBig;
}

void SimStreamer::streamImage()
{
    if(m_showBig) {
        BufPtr bytes = capture_jpg;
        uint32_t len = capture_jpg_len;

        streamFrame(bytes, len);
    }
    else {
        BufPtr bytes = octo_jpg;
        uint32_t len = octo_jpg_len;

        streamFrame(bytes, len);
    }
}
