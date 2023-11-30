#ifndef RTSPSERVER_RTSPCONNECTION_H
#define RTSPSERVER_RTSPCONNECTION_H

#include <cstdint>
#include <map>
#include <string>

#include "MediaSession.h"
#include "RtpInstance.h"
#include "TcpConnection.h"

class RtspServer;
class RtspConnection : public TcpConnection {
public:
    enum Method {
        OPTIONS,
        DESCRIBE,
        SETUP,
        PLAY,
        TEARDOWN,
        NONE
    };

    static RtspConnection* createNew(RtspServer* server, int clientFd);

    RtspConnection(RtspServer* server, int clientFd);
    virtual ~RtspConnection();

protected:
    void handleReadBytes() override;

private:
    bool parseRequest();
    bool parseRequest1(const char* begin, const char* end);
    bool parseRequest2(const char* begin, const char* end);

    bool parseCSeq(std::string& message);
    bool parseDescribe(std::string& message);
    bool parseSetup(std::string& message);
    bool parsePlay(const std::string& message);

    bool handleCmdOptions();
    bool handleCmdDescribe();
    bool handleCmdSetup();
    bool handleCmdPlay();
    bool handleCmdTeardown();

    int sendMessage(void* buf, int size);
    int sendMessage();

    bool createRtpRtcpOverUdp(MediaSession::TrackId trackId, std::string peerIp,
        uint16_t peerRtpPort, uint16_t peerRtcpPort);

    bool createRtpOverTcp(MediaSession::TrackId trackId, int sockfd, uint8_t rtpChannel);

    bool handleRtpOverTcp();

private:
    RtspServer* mRtspServer;
    std::string mPeerIp;
    Method mMethod;
    std::string mUrl;
    std::string mSuffix;
    uint32_t mCSeq;
    std::string mStreamPrefix; // 数据流名称

    uint16_t mPeerRtpPort;
    uint16_t mPeerRtcpPort;

    MediaSession::TrackId mTrackId;
    RtpInstance* mRtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* mRtcpInstances[MEDIA_MAX_TRACK_NUM];

    int mSessionId;
    bool mIsRtpOverTcp;
    uint8_t mRtpChannel;
};

#endif
