#include <cstdint>

#include "RtpInstance.h"

RtpInstance* RtpInstance::createNewOverUdp(
    int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort) {
    return new RtpInstance(localSockfd, localPort, destIp, destPort);
}

RtpInstance* RtpInstance::createNewOverTcp(int sockfd, uint8_t rtpChannel) {
    return new RtpInstance(sockfd, rtpChannel);
}

RtpInstance::RtpInstance(
    int localSockfd, uint16_t localPort, const std::string& destIp, uint16_t destPort)
    : mRtpType(RTP_OVER_UDP)
    , mLocalSockfd(localSockfd)
    , mLocalPort(localPort)
    , mDestAddr(destIp, destPort)
    , mIsAlive(false)
    , mSessionId(0)
    , mRtpChannel(0) {}

RtpInstance::RtpInstance(int sockfd, uint8_t rtpChannel)
    : mRtpType(RTP_OVER_TCP)
    , mLocalSockfd(sockfd)
    , mLocalPort(0)
    , mIsAlive(false)
    , mSessionId(0)
    , mRtpChannel(rtpChannel) {}

RtpInstance::~RtpInstance() {
    sockets::close(mLocalSockfd);
}

int RtpInstance::send(RtpPacket* rtpPacket) {
    switch (mRtpType) {
    case RtpInstance::RTP_OVER_UDP:
        return sendOverUdp(rtpPacket->mBuffer4, rtpPacket->mSize);
        break;
    case RtpInstance::RTP_OVER_TCP:
        rtpPacket->mBuffer[0] = '$';
        rtpPacket->mBuffer[1] = (uint8_t)mRtpChannel;
        rtpPacket->mBuffer[2] = (uint8_t)(((rtpPacket->mSize) & 0xFF00) >> 8);
        rtpPacket->mBuffer[3] = (uint8_t)((rtpPacket->mSize) & 0xFF);
        return sendOverTcp(rtpPacket->mBuffer, rtpPacket->mSize + 4);
        break;
    default:
        return -1;
    }
}

RtcpInstance* RtcpInstance::createNew(
    int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort) {
    return new RtcpInstance(localSockfd, localPort, destIp, destPort);
}

RtcpInstance::RtcpInstance(
    int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort)
    : mLocalSockfd(localSockfd)
    , mLocalPort(localPort)
    , mDestAddr(destIp, destPort)
    , mIsAlive(false)
    , mSessionId(0) {}

RtcpInstance::~RtcpInstance() {
    sockets::close(mLocalSockfd);
}
