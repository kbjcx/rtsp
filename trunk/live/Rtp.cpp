#include "Rtp.h"

#include <cstdint>
#include <cstdlib>

#include <string.h>

RtpPacket::RtpPacket()
    : mBuffer((uint8_t*)malloc(4 + RTP_HEADER_SIZE + RTP_MAX_PACKET_SIZE + 100))
    , mBuffer4(mBuffer + 4)
    , mRtpHeader((RtpHeader*)mBuffer4)
    , mSize(0) {}

RtpPacket::~RtpPacket() {
    free(mBuffer);
    mBuffer = nullptr;
}

void parseRtpHeader(uint8_t* buffer, RtpHeader* rtpHeader) {
    memset(rtpHeader, 0, sizeof(*rtpHeader));

    // byte 1
    rtpHeader->version = (buffer[0] & 0xC0) >> 6;
    rtpHeader->padding = (buffer[0] & 0x20) >> 5;
    rtpHeader->extension = (buffer[0] & 0x10) >> 4;
    rtpHeader->csrcLen = (buffer[0] & 0x0F);

    // byte 2
    rtpHeader->marker = (buffer[1] & 0x80) >> 7;
    rtpHeader->payloadType = (buffer[1] & 0x7F);

    // byte 3-4
    rtpHeader->seq = (((uint16_t)buffer[2]) << 8) | ((uint16_t)buffer[3]);

    // byte 5-8
    rtpHeader->timestamp = ((uint32_t)buffer[4] << 24) | ((uint32_t)buffer[5] << 16) |
                           ((uint32_t)buffer[6] << 8) | ((uint32_t)buffer[7]);

    // byte 9-12
    rtpHeader->ssrc = ((uint32_t)buffer[8] << 24) | ((uint32_t)buffer[9] << 16) |
                      ((uint32_t)buffer[10] << 8) | ((uint32_t)buffer[11]);
}

void parseRtcpHeader(uint8_t* buffer, RtcpHeader* rtcpHeader) {
    memset(rtcpHeader, 0, sizeof(*rtcpHeader));

    // byte 1
    rtcpHeader->version = (buffer[0] & 0xC0) >> 6;
    rtcpHeader->padding = (buffer[0] & 0x20) >> 5;
    rtcpHeader->rc = (buffer[0] & 0x1F);

    // byte 2
    rtcpHeader->packetType = buffer[1];

    // byte 3-4
    rtcpHeader->length = ((uint16_t)buffer[2] << 8) | ((uint16_t)buffer[3]);
}
 