#ifndef RTSPSERVER_RTP_H
#define RTSPSERVER_RTP_H

#include <cstdint>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define RTP_VERSION           2
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC  97
#define RTP_HEADER_SIZE       12
#define RTP_MAX_PACKET_SIZE   1400

/**
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|CC=4   |M|PT=7         |sequence number=16             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           time stamp                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            synchronixation source identifier(SSRC)            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             contributing source identifiers(CSRC)             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       [CSRC(0 ~ 15个)]						|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   payload(video, audio ...)                   |
+                                           +-+-+-+-+-+-+-+-+-+-+
|                                           |       padding     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**/
struct RtpHeader {
    // byte 1
    uint8_t csrcLen     : 4;
    uint8_t extension   : 1;
    uint8_t padding     : 1;
    uint8_t version     : 2;

    // byte 2
    uint8_t payloadType : 7;
    uint8_t marker      : 1;

    // byte 3-4
    uint16_t seq;

    // byte 5-8
    uint32_t timestamp;

    // byte 9-12
    uint32_t ssrc;

    // data
    uint8_t payload[0];
};

struct RtcpHeader {
    // byte 1
    uint8_t rc      : 5;
    uint8_t padding : 1;
    uint8_t version : 2;

    // byte 2
    uint8_t packetType;

    // byte 3-4
    uint16_t length;
};

class RtpPacket {
public:
    RtpPacket();
    ~RtpPacket();

public:
    uint8_t* mBuffer;  // 4 + RtpHeader + data
    uint8_t* mBuffer4; // RtpHeader + data
    RtpHeader* const mRtpHeader;
    int mSize; // RtpHeader + data
};

void parseRtpHeader(uint8_t* buffer, RtpHeader* rtpHeader);
void parseRtcpHeader(uint8_t* buffer, RtcpHeader* rtcpHeader);

#endif // !RTSPSERVER_RTP_H
