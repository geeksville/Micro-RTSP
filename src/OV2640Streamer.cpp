
#include "OV2640Streamer.h"
#include <assert.h>



OV2640Streamer::OV2640Streamer(SOCKET aClient, OV2640 &cam) : CStreamer(aClient, cam.getWidth(), cam.getHeight()), m_cam(cam)
{
}

void OV2640Streamer::streamImage(uint32_t curMsec)
{
    m_cam.run();// queue up a read for next time

    BufPtr bytes = m_cam.getfb();
    streamFrame(bytes, m_cam.getSize(), curMsec);
}
