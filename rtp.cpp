#include "rtp.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

void rtpHeaderInit(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker, uint16_t seq,
    uint32_t timestamp, uint32_t ssrc) {
    rtpPacket->rtpHeader.csrcLen = csrcLen;
    rtpPacket->rtpHeader.extension = extension;
    rtpPacket->rtpHeader.padding = padding;
    rtpPacket->rtpHeader.version = version;
    rtpPacket->rtpHeader.payloadType = payloadType;
    rtpPacket->rtpHeader.marker = marker;
    rtpPacket->rtpHeader.seq = seq;
    rtpPacket->rtpHeader.timestamp = timestamp;
    rtpPacket->rtpHeader.ssrc = ssrc;
}

int rtpSendPacketOverTcp(
    int clientSockfd, RtpPacket* rtpPacket, uint32_t datasize, char channel) {
    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htons(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htons(rtpPacket->rtpHeader.ssrc);

    uint32_t rtpSize = RTP_HEADER_SIZE + datasize;
    char* tempBuffer = (char*)malloc(rtpSize + 4);

    tempBuffer[0] = 0x24;
    tempBuffer[1] = channel;
    tempBuffer[2] = (uint8_t)((rtpSize & 0xFF00) >> 8);
    tempBuffer[3] = (uint8_t)(rtpSize & 0xFF);

    memcpy(tempBuffer + 4, (char*)rtpPacket, rtpSize);

    int ret = send(clientSockfd, tempBuffer, rtpSize + 4, 0);

    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohs(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohs(rtpPacket->rtpHeader.ssrc);

    free(tempBuffer);
    tempBuffer = nullptr;

    return ret;
}

int rtpSendPacketOverUdp(int serverRtpSockfd, const char* ip, int16_t port,
    RtpPacket* rtpPacket, uint32_t datasize) {
    sockaddr_in addr;
    int ret;

    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(ip);

    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htons(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htons(rtpPacket->rtpHeader.ssrc);

    ret = sendto(serverRtpSockfd, (char*)rtpPacket, datasize + RTP_HEADER_SIZE, 0,
        (sockaddr*)&addr, sizeof(addr));

    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohs(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohs(rtpPacket->rtpHeader.ssrc);

    return ret;
}
