#include "AudioStreamer.h"
#include <stdio.h>


AudioStreamer::AudioStreamer()
{
    printf("Creating Audio streamer\n");
    m_RtpServerPort  = 0;
    m_RtcpServerPort = 0;

    m_SequenceNumber = 0;
    m_Timestamp      = 0;
    m_SendIdx        = 0;

    m_RtpSocket = NULLSOCKET;
    m_RtcpSocket = NULLSOCKET;

    m_prevMsec = 0;

    m_udpRefCount = 0;

    /*
    m_streamingData = xQueueCreate(STREAMING_BUFFER_SIZE, sizeof(SAMPLE_TYPE));
    if (m_streamingData == NULL) {
        printf("ERROR: Queue for streaming data could not be created\n");
    }
    */

    if (xTaskCreatePinnedToCore(doRTPStream, "RTPTask", 8192, (void*)this, 10, &m_RTPTask, 0) != pdPASS) {
        printf("ERROR: Task for streaming data could not be created\n");   
    }
    
    this->m_fragmentSize = m_samplingRate / 50;
    this->m_fragmentSizeBytes = m_fragmentSize * m_sampleSizeBytes;

    printf("Audio streamer created. Sampling rate: %i, Fragment size: %i (%i bytes)\n", m_samplingRate, m_fragmentSize, m_fragmentSizeBytes);
}

AudioStreamer::AudioStreamer(IAudioSource * source) : AudioStreamer() {
    this->m_audioSource = source;
    this->m_samplingRate = source->getSampleRate();
    this->m_sampleSizeBytes = source->getSampleSizeBytes();
}

AudioStreamer::~AudioStreamer()
{
    
}

int AudioStreamer::getSampleRate() {
    return this->m_samplingRate;
}

/*
template<class SAMPLE_TYPE>
int AudioStreamer<SAMPLE_TYPE>::SendRtpPacket(SAMPLE_TYPE * data, int len)
{
    static char RtpBuf[2048]; // Note: we assume single threaded, this large buf we keep off of the tiny stack
    if (len > m_fragmentSize) len = m_fragmentSize;
    int lenBytes = len * sizeof(SAMPLE_TYPE);

    int RtpPacketSize = lenBytes + HEADER_SIZE;

    memset(RtpBuf, 0x00, sizeof(RtpBuf));

    // Prepare the 12 byte RTP header
    RtpBuf[0]  = 0x80;                               // RTP version
    RtpBuf[1]  = 0x0b | 0x80;                               // L16 payload (11) and no marker bit
    RtpBuf[3]  = m_SequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
    RtpBuf[2]  = m_SequenceNumber >> 8;
    RtpBuf[4]  = (m_Timestamp & 0xFF000000) >> 24;   // each image gets a timestamp
    RtpBuf[5]  = (m_Timestamp & 0x00FF0000) >> 16;
    RtpBuf[6] = (m_Timestamp & 0x0000FF00) >> 8;
    RtpBuf[7] = (m_Timestamp & 0x000000FF);
    RtpBuf[8] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
    RtpBuf[9] = 0xf9;                               // we just an arbitrary number here to keep it simple
    RtpBuf[10] = 0x7e;
    RtpBuf[11] = 0x67;

    int headerLen = 12; 

    // append data to header
    memcpy(RtpBuf + headerLen, (void*) data, lenBytes);

    m_SequenceNumber++;                              // prepare the packet counter for the next packet

    //IPADDRESS otherip;
    //IPPORT otherport;

    // RTP marker bit must be set on last fragment
    //if (m_Client->m_streaming && !session->m_stopped) {
        // UDP - we send just the buffer by skipping the 4 byte RTP over RTSP header
        //socketpeeraddr(session->getClient(), &otherip, &otherport);
    udpsocketsend(m_RtpSocket, RtpBuf, RtpPacketSize, m_ClientIP, m_ClientPort);
    //}

    // printf("CStreamer::SendRtpPacket offset:%d - end\n", fragmentOffset);
    return len;
}
*/

int AudioStreamer::SendRtpPacketDirect() {
    static unsigned char RtpBuf[2048]; // Note: we assume single threaded, this large buf we keep off of the tiny stack

    // Prepare the 12 byte RTP header
    RtpBuf[0]  = 0x80;                               // RTP version
    RtpBuf[1]  = 0x0b | 0x80;                               // L16 payload (11) and no marker bit
    RtpBuf[3]  = m_SequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
    RtpBuf[2]  = m_SequenceNumber >> 8;
    RtpBuf[4]  = (m_Timestamp & 0xFF000000) >> 24;   // each image gets a timestamp
    RtpBuf[5]  = (m_Timestamp & 0x00FF0000) >> 16;
    RtpBuf[6] = (m_Timestamp & 0x0000FF00) >> 8;
    RtpBuf[7] = (m_Timestamp & 0x000000FF);
    RtpBuf[8] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
    RtpBuf[9] = 0xf9;                               // we just an arbitrary number here to keep it simple
    RtpBuf[10] = 0x7e;
    RtpBuf[11] = 0x67;

    // append data to header
    if (m_audioSource == NULL) {
        log_e("No audio source provided");
        return -1;
    }
    unsigned char * dataBuf = (RtpBuf + HEADER_SIZE);
    //printf("Trying to read %i bytes from streaming source\n", m_fragmentSize);
    int samples = m_audioSource->readDataTo(dataBuf, m_fragmentSize);
    int byteLen = samples * m_audioSource->getSampleSizeBytes();

    m_SequenceNumber++;                              // prepare the packet counter for the next packet

    udpsocketsend(m_RtpSocket, RtpBuf, HEADER_SIZE + byteLen, m_ClientIP, m_ClientPort);

    return samples;
}

u_short AudioStreamer::GetRtpServerPort()
{
    return m_RtpServerPort;
};

u_short AudioStreamer::GetRtcpServerPort()
{
    return m_RtcpServerPort;
};

bool AudioStreamer::InitUdpTransport(IPADDRESS aClientIP, IPPORT aClientPort)
{
    m_ClientIP = aClientIP;
    m_ClientPort = aClientPort;

    m_SequenceNumber = getRandom();

    if (m_udpRefCount != 0)
    {
        ++m_udpRefCount;
        return true;
    }

    for (u_short P = 6970; P < 0xFFFE; P += 2)
    {
        m_RtpSocket     = udpsocketcreate(P);
        if (m_RtpSocket)
        {   // Rtp socket was bound successfully. Lets try to bind the consecutive Rtsp socket
            m_RtcpSocket = udpsocketcreate(P + 1);
            if (m_RtcpSocket)
            {
                m_RtpServerPort  = P;
                m_RtcpServerPort = P+1;
                break;
            }
            else
            {
                udpsocketclose(m_RtpSocket);
                udpsocketclose(m_RtcpSocket);
            };
        }
    };
    ++m_udpRefCount;

    printf("RTP Streamer set up with client IP %s and client Port %i\n", inet_ntoa(m_ClientIP), m_ClientPort);

    return true;
}

void AudioStreamer::ReleaseUdpTransport(void)
{
    --m_udpRefCount;
    if (m_udpRefCount == 0)
    {
        m_RtpServerPort  = 0;
        m_RtcpServerPort = 0;
        udpsocketclose(m_RtpSocket);
        udpsocketclose(m_RtcpSocket);

        m_RtpSocket = NULLSOCKET;
        m_RtcpSocket = NULLSOCKET;
    }
}

/*
int AudioStreamer::AddToStream(SAMPLE_TYPE * data, int len) {
    for (int i=0; i < len; i++) {
        if (xQueueSend(m_streamingData, &data[i], 0) != pdTRUE) {
            return i;
        }
    }

    return 0;
}
*/

void AudioStreamer::Start() {
    printf("Starting RTP Stream\n");
    if (m_audioSource != NULL) {
        m_audioSource->start();
        vTaskResume(m_RTPTask);
    } else {
        printf("Error: no streaming source\n");
    }
}

void AudioStreamer::Stop() {
    printf("Stopping RTP Stream\n");
    if (m_audioSource != NULL) {
        m_audioSource->stop();
    }
    vTaskSuspend(m_RTPTask);
}

void AudioStreamer::doRTPStream(void * audioStreamerObj) {
    AudioStreamer * streamer = (AudioStreamer*)audioStreamerObj;
    int samples;
    TickType_t prevWakeTime =  xTaskGetTickCount();


    vTaskSuspend(NULL);     // only start when Start() is called

    while(1) {
        
        samples = streamer->SendRtpPacketDirect();
        //int after = xTaskGetTickCount();
        //printf("RTP packet sent at %i\n", after);
        if (samples < 0) {
            printf("Direct sending of RTP stream failed\n");
        } else if (samples > 0) {           // samples have been sent
            streamer->m_Timestamp += samples;        // no of samples sent
            //printf("%i samples sent (%ims); timestamp: %i\n", samples, samples / 16, streamer->m_Timestamp);

            // delay a little less than 20ms
            if (prevWakeTime + 20/portTICK_PERIOD_MS > xTaskGetTickCount()) {
                printf("RTP Task is too slow!\n");
                // reset prevWakeTime to not get this message repeatedly
                prevWakeTime = xTaskGetTickCount() - 20/portTICK_PERIOD_MS;
            }
            vTaskDelayUntil(&prevWakeTime, 17/portTICK_PERIOD_MS);
        }
        
    }

    printf("Error: %s is returning\n", pcTaskGetTaskName(NULL));
}