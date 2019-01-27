
#include "SimStreamer.h"
#include "JPEGSamples.h"
#include <assert.h>

typedef unsigned const char *BufPtr;

// When JPEG is stored as a file it is wrapped in a container
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
unsigned decodeJPEGfile(BufPtr *start) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    unsigned const char *bytes = *start;

    assert(bytes[0] == 0xff);
    assert(bytes[1] == 0xd8);

    // kinda skanky, will break if unlucky and the headers inxlucde 0xffda
    // might fall off array if jpeg is invalid
    while(true) {
        while(*bytes++ != 0xff)
            ;
        if(*bytes++ == 0xda) {
            unsigned skipped = bytes - *start;
            // printf("first byte %x, skipped %d\n", *bytes, skipped);

            *start = bytes;
            return skipped;
        }
    }

    // if we have to properly parse headers
    // unsigned len = bytes[2] * 256 + bytes[3] - 2;
    // *start = bytes + 4;
}

SimStreamer::SimStreamer(SOCKET aClient, bool showBig) : CStreamer(aClient, showBig ? 640 : 64, showBig ? 480 : 48)
{
    m_showBig = showBig;
}

void SimStreamer::streamImage()
{
    static int frameoffset = 0;

    if(m_showBig) {
        BufPtr bytes = octo_jpg;
        unsigned skipped = decodeJPEGfile(&bytes);
        streamFrame(bytes, octo_jpg_len - skipped);
    }
    else {
        unsigned const char  * Samples2[2] = { JpegScanDataCh2A, JpegScanDataCh2B };

        streamFrame(Samples2[frameoffset], KJpegCh2ScanDataLen);
        frameoffset = (frameoffset + 1) % 2;
    }
}
