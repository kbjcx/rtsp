#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "../base/log.h"
#include "../base/version.h"
#include "MediaSession.h"
#include "MediaSessionManager.h"
#include "RtspConnection.h"
#include "RtspServer.h"
#include "TcpConnection.h"

static void getPeerIp(int fd, std::string& ip) {
    sockaddr_in addr;
    socklen_t addrLen = sizeof(sockaddr_in);
    getpeername(fd, (sockaddr*)&addr, &addrLen);
    ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::createNew(RtspServer* server, int clientFd) {
    return new RtspConnection(server, clientFd);
}

RtspConnection::RtspConnection(RtspServer* server, int clientFd)
    : TcpConnection(server->env(), clientFd)
    , mRtspServer(server)
    , mMethod(RtspConnection::Method::NONE)
    , mTrackId(MediaSession::TRACK_ID_NONE)
    , mSessionId(rand())
    , mIsRtpOverTcp(false)
    , mStreamPrefix("track") {
    LOGINFO("RtspConnection() mClientFd = %d", mClientFd);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        mRtpInstances[i] = nullptr;
        mRtcpInstances[i] = nullptr;
    }

    getPeerIp(clientFd, mPeerIp);
}

RtspConnection::~RtspConnection() {
    LOGINFO("~RtspConnection() mClientFd = %d", mClientFd);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (mRtpInstances[i]) {
            MediaSession* session = mRtspServer->mSessionManager->getSession(mSuffix);

            if (!session) {
                session->removeRtpInstance(mRtpInstances[i]);
            }

            delete mRtpInstances[i];
            mRtpInstances[i] = nullptr;
        }

        if (mRtcpInstances[i]) {
            delete mRtcpInstances[i];
            mRtcpInstances[i] = nullptr;
        }
    }
}

void RtspConnection::handleReadBytes() {
    if (mIsRtpOverTcp) {
        if (mInputBuffer.peek()[0] == '$') {
            handleRtpOverTcp();
            return;
        }
    }

    if (!parseRequest()) {
        LOGERROR("parse request error");
        goto disconnect;
    }

    switch (mMethod) {
    case OPTIONS:
        if (!handleCmdOptions()) {
            goto disconnect;
        }
        break;
    case DESCRIBE:
        if (!handleCmdDescribe()) {
            goto disconnect;
        }
        break;
    case SETUP:
        if (!handleCmdSetup()) {
            goto disconnect;
        }
        break;
    case PLAY:
        if (!handleCmdPlay()) {
            goto disconnect;
        }
        break;
    case TEARDOWN:
        if (!handleCmdTeardown()) {
            goto disconnect;
        }
        break;
    default:
        goto disconnect;
        break;
    }

    return;

disconnect:
    handleDisConnect();
}

bool RtspConnection::parseRequest() {
    // parse first line
    const char* crlf = mInputBuffer.findCRLF();
    if (crlf == nullptr) {
        mInputBuffer.retrieveAll();
        return false;
    }

    bool ret = parseRequest1(mInputBuffer.peek(), crlf);
    if (ret == false) {
        mInputBuffer.retrieveAll();
        return false;
    } else {
        mInputBuffer.retrieveUntil(crlf + 2);
    }

    // parse lines after first line
    crlf = mInputBuffer.findLastCRLF();
    if (crlf == nullptr) {
        mInputBuffer.retrieveAll();
        return false;
    }

    ret = parseRequest2(mInputBuffer.peek(), crlf);

    if (ret == false) {
        mInputBuffer.retrieveAll();
        return false;
    } else {
        mInputBuffer.retrieveUntil(crlf + 2);
        return true;
    }
}

bool RtspConnection::parseRequest1(const char* begin, const char* end) {
    std::string message(begin, end);
    char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};

    if (sscanf(message.data(), "%s %s %s", method, url, version) != 3) {
        return false;
    }

    if (!strcmp(method, "OPTIONS")) {
        mMethod = OPTIONS;
    } else if (!strcmp(method, "DESCRIBE")) {
        mMethod = DESCRIBE;
    } else if (!strcmp(method, "SETUP")) {
        mMethod = SETUP;
    } else if (!strcmp(method, "PLAY")) {
        mMethod = PLAY;
    } else if (!strcmp(method, "TEARDOWN")) {
        mMethod = TEARDOWN;
    } else {
        mMethod = NONE;
        return false;
    }

    if (strncmp(url, "rtsp://", 7) != 0) {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = {0};
    char suffix[64] = {0};
    int ret = sscanf(url + 6, "%[^:]:%hu/%s", ip, &port, suffix);
    if (ret == 2) {
        port == 8554; // 如果没有获取到端口， 默认为8554
    } else if (ret != 3) {
        return false;
    }

    mUrl = url;
    mSuffix = suffix;

    return true;
}

bool RtspConnection::parseRequest2(const char* begin, const char* end) {
    std::string message(begin, end);

    if (!parseCSeq(message)) {
        return false;
    }

    if (mMethod == OPTIONS) {
        return true;
    } else if (mMethod == DESCRIBE) {
        return parseDescribe(message);
    } else if (mMethod == SETUP) {
        return parseSetup(message);
    } else if (mMethod == PLAY) {
        return parsePlay(message);
    } else if (mMethod == TEARDOWN) {
        return true;
    } else {
        return false;
    }
}

bool RtspConnection::parseCSeq(std::string& message) {
    size_t pos = message.find("CSeq");
    if (pos != std::string::npos) {
        uint32_t cseq = 0;
        sscanf(message.data() + pos, "%*[^:]: %u", &cseq);
        mCSeq = cseq;
        return true;
    }

    return false;
}

bool RtspConnection::parseDescribe(std::string& message) {
    if ((message.rfind("Accept") == std::string::npos) ||
        (message.rfind("sdp") == std::string::npos)) {
        return false;
    }

    return true;
}

bool RtspConnection::parseSetup(std::string& message) {
    mTrackId = MediaSession::TRACK_ID_NONE;
    size_t pos;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        pos = mUrl.find(mStreamPrefix + std::to_string(i));
        if (pos != std::string::npos) {
            if (i == 0) {
                mTrackId = MediaSession::TRACK_ID_0;
            } else if (i == 1) {
                mTrackId = MediaSession::TRACK_ID_1;
            }
        }
    }

    if (mTrackId == MediaSession::TRACK_ID_NONE) {
        return false;
    }

    pos = message.find("Transport");
    if (pos != std::string::npos) {
        if ((pos = message.find("RTP/AVP/TCP")) != std::string::npos) {
            uint8_t rtpChannel, rtcpChannel;
            mIsRtpOverTcp = true;

            if (sscanf(message.data(), "%*[^;];%*[^;];%*[^=]=%hhu-%hhu", &rtpChannel,
                    &rtcpChannel) != 2) {
                return false;
            }

            mRtpChannel = rtpChannel;

            return true;
        } else if ((pos = message.find("RTP/AVP")) != std::string::npos) {
            uint16_t rtpPort = 0, rtcpPort = 0;
            if ((pos = message.find("unicast")) != std::string::npos) {
                if (sscanf(message.data() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu", &rtpPort,
                        &rtcpPort) != 2) {
                    return false;
                }
            } else if ((pos = message.find("multicast")) != std::string::npos) {
                return false;
            } else {
                return false;
            }

            mPeerRtpPort = rtpPort;
            mPeerRtcpPort = rtcpPort;
        } else {
            return false;
        }

        return true;
    }

    return false;
}

bool RtspConnection::parsePlay(std::string& message) {
    size_t pos = message.find("Session");
    if (pos != std::string::npos) {
        uint32_t sessionId = 0;
        if (sscanf(message.data() + pos, "%*[^:]: %u", &sessionId) != 1) {
            return false;
        }
        return true;
    }

    return false;
}

bool RtspConnection::handleCmdOptions() {
    snprintf(mBuffer, sizeof(mBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, "
        "TEARDOWN\r\n"
        "Server: %s\r\n"
        "\r\n",
        mCSeq, PROJECT_VERSION);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdDescribe() {
    MediaSession* session = mRtspServer->mSessionManager->getSession(mSuffix);

    if (!session) {
        LOGERROR("can not find session: %s", mSuffix.data());
        return false;
    }

    std::string sdp = session->generateSPDDescription();

    memset((void*)mBuffer, 0, sizeof(mBuffer));
    snprintf((char*)mBuffer, sizeof(mBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: application/sdp\r\n"
        "\r\n"
        "%s",
        mCSeq, (unsigned)sdp.size(), sdp.data());

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdSetup() {
    char sessionName[100];
    if (sscanf(mSuffix.data(), "%[^/]/", sessionName) != 1) {
        return false;
    }

    MediaSession* session = mRtspServer->mSessionManager->getSession(sessionName);
    if (!session) {
        LOGERROR("can not find session: %s", sessionName);
        return false;
    }

    if (mTrackId >= MEDIA_MAX_TRACK_NUM || mRtpInstances[mTrackId] ||
        mRtcpInstances[mTrackId]) {
        return false;
    }

    if (session->isStartMulticast()) {
        snprintf((char*)mBuffer, sizeof(mBuffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP;multicast;"
            "destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
            "Session: %08x\r\n"
            "\r\n",
            mCSeq, session->getMulticastDestAddr().data(), sockets::getLocalIp().data(),
            session->getMulticastDestRtpPort(mTrackId),
            session->getMulticastDestRtpPort(mTrackId) + 1, mSessionId);
    } else {
        if (mIsRtpOverTcp) {
            createRtpOverTcp(mTrackId, mClientFd, mRtpChannel);
            mRtpInstances[mTrackId]->setSessionId(mSessionId);

            session->addRtpInstance(mTrackId, mRtpInstances[mTrackId]);

            snprintf((char*)mBuffer, sizeof(mBuffer),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                mCSeq, PROJECT_VERSION, mRtpChannel, mRtpChannel + 1, mSessionId);
        } else {
            if (!createRtpRtcpOverUdp(mTrackId, mPeerIp, mPeerRtpPort, mPeerRtcpPort)) {
                LOGERROR("failed to create rtp/rtcp over udp");
                return false;
            }

            mRtpInstances[mTrackId]->setSessionId(mSessionId);
            mRtcpInstances[mTrackId]->setSessionId(mSessionId);

            session->addRtpInstance(mTrackId, mRtpInstances[mTrackId]);

            snprintf((char*)mBuffer, sizeof(mBuffer),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %u\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                mCSeq, PROJECT_VERSION, mPeerRtpPort, mPeerRtcpPort,
                mRtpInstances[mTrackId]->getLocalPort(),
                mRtcpInstances[mTrackId]->getLocalPort(), mSessionId);
        }
    }

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::handleCmdPlay() {
    snprintf((char*)mBuffer, sizeof(mBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %08x; timeout=60\r\n"
        "\r\n",
        mCSeq, PROJECT_VERSION, mSessionId);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0) {
        return false;
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (mRtpInstances[i]) {
            mRtpInstances[i]->setAlive(true);
        }

        if (mRtcpInstances[i]) {
            mRtcpInstances[i]->setAlive(true);
        }
    }

    return true;
}

bool RtspConnection::handleCmdTeardown() {
    snprintf((char*)mBuffer, sizeof(mBuffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "\r\n",
        mCSeq, PROJECT_VERSION);

    if (sendMessage(mBuffer, strlen(mBuffer)) < 0) {
        return false;
    }

    return true;
}

int RtspConnection::sendMessage(void* buf, int size) {
    LOGINFO("%s", buf);
    int ret;
    mOutputBuffer.append(buf, size);
    ret = mOutputBuffer.write(mClientFd);
    mOutputBuffer.retrieveAll();

    return ret;
}

int RtspConnection::sendMessage() {
    int ret = mOutputBuffer.write(mClientFd);
    mOutputBuffer.retrieveAll();
    return ret;
}

bool RtspConnection::createRtpRtcpOverUdp(MediaSession::TrackId trackId,
    std::string peerIp, uint16_t peerRtpPort, uint16_t peerRtcpPort) {
    int rtpSocket, rtcpSocket;
    uint16_t rtpPort, rtcpPort;
    bool ret;

    if (mRtpInstances[trackId] || mRtcpInstances[trackId]) {
        return false;
    }

    int i;
    for (i = 0; i < 10; ++i) {
        rtpSocket = sockets::createUdpSocket();
        if (rtpSocket < 0) {
            return false;
        }

        rtcpSocket = sockets::createUdpSocket();
        if (rtcpSocket < 0) {
            sockets::close(rtpSocket);
            return false;
        }

        uint16_t port = rand() & 0xFFFE;
        if (port < 10000) {
            port += 10000;
        }

        rtpPort = port;
        rtcpPort = port + 1;

        ret = sockets::bind(rtpSocket, "0.0.0.0", rtpPort);
        if (!ret) {
            sockets::close(rtpSocket);
            sockets::close(rtcpSocket);
            continue;
        }

        break;
    }

    if (i == 10) {
        return false;
    }

    mRtpInstances[trackId] =
        RtpInstance::createNewOverUdp(rtpSocket, rtpPort, peerIp, peerRtpPort);
    mRtcpInstances[trackId] =
        RtcpInstance::createNew(rtcpSocket, rtcpPort, peerIp, peerRtpPort);

    return true;
}

bool RtspConnection::createRtpOverTcp(
    MediaSession::TrackId trackId, int sockfd, uint8_t rtpChannel) {
    mRtpInstances[trackId] = RtpInstance::createNewOverTcp(sockfd, rtpChannel);

    return true;
}

void RtspConnection::handleRtpOverTcp() {
    int num = 0;
    while (true) {
        num += 1;
        uint8_t* buf = (uint8_t*)mInputBuffer.peek();
        uint8_t rtpChannel = buf[1];
        int16_t rtpSize = (buf[2] << 8) | buf[3];
        int16_t bufSize = rtpSize + 4;

        if (mInputBuffer.readableBytes() < bufSize) {
            // cached data is less than the length of RTP header
            return;
        } else {
            if (rtpChannel == 0x00) {
                RtpHeader rtpHeader;
                parseRtpHeader(buf + 4, &rtpHeader);
                LOGINFO("num=%d, rtpSize=%d", num, rtpSize);
            } else if (rtpChannel == 0x01) {
                RtcpHeader rtcpHeader;
                parseRtcpHeader(buf + 4, &rtcpHeader);

                LOGINFO("num=%d, rtcpHeader.packetType=%d, rtpSize=%d", num,
                    rtcpHeader.packetType, rtpSize);
            } else if (rtpChannel == 0x02) {
                RtpHeader rtpHeader;
                parseRtpHeader(buf + 4, &rtpHeader);
                LOGINFO("num=%d, rtpSize=%d", num, rtpSize);
            } else if (rtpChannel == 0x03) {
                RtcpHeader rtcpHeader;
                parseRtcpHeader(buf + 4, &rtcpHeader);
                LOGINFO("num=%d, rtcpHeader.packetType=%d, rtpSize=%d", num,
                    rtcpHeader.packetType, rtpSize);
            }

            mInputBuffer.retrieve(bufSize);
        }
    }
}
