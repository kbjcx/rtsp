#ifndef RTP_H
#define RTP_H

#include <cstdint>

#define RTP_VERSION           2
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC  97
#define RTP_HEADER_SIZE       12
#define RTP_MAX_PACKET_SIZE   1400

struct RtpHeader {
    // byte 0
    uint8_t csrcLen     : 4;
    uint8_t extension   : 1;
    uint8_t padding     : 1;
    uint8_t version     : 2;

    // byte 1
    uint8_t payloadType : 7;
    uint8_t marker      : 1;

    // byte 2 - 3
    uint16_t seq;

    // byte 4 - 7
    uint32_t timestamp;

    // byte 8 - 11
    uint32_t ssrc;
    // TODO CSRC
}; // struct RtpHeader

struct RtpPacket {
    RtpHeader rtpHeader;
    uint8_t payload[0];
}; // struct RtpPacket

void rtpHeaderInit(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker, uint16_t seq,
    uint32_t timestamp, uint32_t ssrc);

int rtpSendPacketOverTcp(
    int clientSockfd, RtpPacket* rtpPacket, uint32_t dataSize, char channel);

int rtpSendPacketOverUdp(int serverRtpSockfd, const char* ip, int16_t port,
    RtpPacket* rtpPacket, uint32_t dataSize);

#endif
