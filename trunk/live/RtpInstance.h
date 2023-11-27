#ifndef RTSPSERVER_RTPINSTANNCE_H
#define RTSPSERVER_RTPINSTANNCE_H

#include <stdint.h>
#include <cstdint>
#include <string>

#ifndef WIN32
#include <unistd.h>
#endif

#include "../scheduler/SocketsOptions.h"
#include "InetAddress.h"
#include "Rtp.h"

class RtpInstance {
public:
    enum RtpType {
        RTP_OVER_UDP,
        RTP_OVER_TCP
    };

    static RtpInstance* createNewOverUdp(
        int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort);

    static RtpInstance* createNewOverTcp(int sockfd, uint8_t rtpChannel);

    RtpInstance(int localSockfd, uint16_t localPort, const std::string& destIp,
        uint16_t destPort);
    RtpInstance(int sockfd, uint8_t rtpChannel);

    ~RtpInstance();

    uint16_t getLocalPort() const {
        return mLocalPort;
    }
    uint16_t getPeerPort() {
        return mDestAddr.getPort();
    }

    int send(RtpPacket* rtpPacket);

    bool alive() const {
        return mIsAlive;
    }

    int setAlive(bool alive) {
        mIsAlive = alive;
        return 0;
    }

    void setSessionId(uint16_t sessionId) {
        mSessionId = sessionId;
    }

    uint16_t sessionId() const {
        return mSessionId;
    }

private:
    int sendOverUdp(void* buf, int size) {
        return sockets::sendto(mLocalSockfd, buf, size, mDestAddr.getAddr());
    }

    int sendOverTcp(void* buf, int size) {
        return sockets::write(mLocalSockfd, buf, size);
    }

private:
    RtpType mRtpType;
    int mLocalSockfd;
    uint16_t mLocalPort;
    Ipv4Address mDestAddr;
    bool mIsAlive;
    uint16_t mSessionId;
    uint8_t mRtpChannel;
};

class RtcpInstance {
public:
    static RtcpInstance* createNew(
        int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort);

    RtcpInstance(
        int localSockfd, uint16_t localPort, std::string destIp, uint16_t destPort);

    ~RtcpInstance();

    int send(void* buf, int size);

    int recv(void* buf, int size, Ipv4Address* addr);

    uint16_t getLocalPort() const;

    int alive() const;
    int setAlive(bool alive);
    void setSessionId(uint16_t sessionId);
    uint16_t sessionId() const;

private:
    int mLocalSockfd;
    uint16_t mLocalPort;
    Ipv4Address mDestAddr;
    bool mIsAlive;
    uint16_t mSessionId;
};

#endif
