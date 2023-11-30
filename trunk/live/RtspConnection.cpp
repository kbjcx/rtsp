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
}
