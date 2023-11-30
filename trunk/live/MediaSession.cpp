#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "../base/log.h"
#include "MediaSession.h"

MediaSession* MediaSession::createNew(std::string sessionName) {
    return new MediaSession(sessionName);
}

MediaSession::MediaSession(const std::string& sessionName)
    : mSessionName(sessionName)
    , mIsStartMulticast(false) {
    LOGINFO("MediaSession() name = %s", sessionName.c_str());

    mTracks[0].mTrackId = TRACK_ID_0;
    mTracks[0].mIsAlive = false;

    mTracks[1].mTrackId = TRACK_ID_1;
    mTracks[1].mIsAlive = false;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        mMulticastRtpInstances[i] = nullptr;
        mMulticastRtcpInstances[i] = nullptr;
    }
}

MediaSession::~MediaSession() {
    LOGINFO("~MediaSession()");
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (mMulticastRtpInstances[i]) {
            this->removeRtpInstance(mMulticastRtpInstances[i]);
            delete mMulticastRtpInstances[i];
        }

        if (mMulticastRtcpInstances[i]) {
            delete mMulticastRtcpInstances[i];
        }
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (mTracks[i].mIsAlive) {
            auto sink = mTracks[i].mSink;
            delete sink;
            mTracks[i].mSink = nullptr;
        }
    }
}

std::string MediaSession::generateSPDDescription() {
    if (!mSdp.empty()) {
        return mSdp;
    }

    std::string ip = "0.0.0.0";
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
        "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "a=type:broadcast\r\n",
        (long)time(nullptr), ip.data());

    if (isStartMulticast()) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
            "a=rtcp-unicast: reflection\r\n");
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        uint16_t port = 0;
        if (!mTracks[i].mIsAlive) {
            continue;
        }

        if (isStartMulticast()) {
            port = getMulticastDestRtpPort((TrackId)i);
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\r\n",
            mTracks[i].mSink->getMediaDescription(port).data());

        if (isStartMulticast()) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "c=IN IP4 %s/255\r\n",
                getMulticastDestAddr().data());
        } else {
            snprintf(
                buf + strlen(buf), sizeof(buf) - strlen(buf), "c=IN IP4 0.0.0.0\r\n");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\r\n",
            mTracks[i].mSink->getAttribute().data());
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "a=control:track%d\r\n",
            mTracks[i].mTrackId);
    }

    mSdp = buf;
    return mSdp;
}

MediaSession::Track* MediaSession::getTrack(MediaSession::TrackId trackId) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (mTracks[i].mTrackId == trackId) {
            return &mTracks[i];
        }
    }

    return nullptr;
}

bool MediaSession::addSink(MediaSession::TrackId trackId, Sink* sink) {
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }

    track->mSink = sink;
    track->mIsAlive = true;

    sink->setSessionCallback(MediaSession::sendPacketCallback, this, track);
    return true;
}

bool MediaSession::addRtpInstance(
    MediaSession::TrackId trackId, RtpInstance* rtpInstance) {
    auto track = getTrack(trackId);

    if (!track || !track->mIsAlive) {
        return false;
    }

    track->mRtpInstances.push_back(rtpInstance);

    return true;
}

bool MediaSession::removeRtpInstance(RtpInstance* rtpInstance) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (!mTracks[i].mIsAlive) {
            continue;
        }

        auto it = std::find(mTracks[i].mRtpInstances.begin(),
            mTracks[i].mRtpInstances.end(), rtpInstance);
        if (it == mTracks[i].mRtpInstances.end()) {
            continue;
        }

        mTracks[i].mRtpInstances.erase(it);

        return true;
    }

    return false;
}

void MediaSession::sendPacketCallback(
    void* arg1, void* arg2, void* packet, Sink::PacketType packetType) {
    auto rtpPacket = (RtpPacket*)packet;
    auto mediaSession = (MediaSession*)arg1;
    auto track = (MediaSession::Track*)arg2;

    mediaSession->handleSendRtpPacket(track, rtpPacket);
}

void MediaSession::handleSendRtpPacket(MediaSession::Track* track, RtpPacket* rtpPacket) {
    std::list<RtpInstance*>::iterator it;
    for (it = track->mRtpInstances.begin(); it != track->mRtpInstances.end(); ++it) {
        if ((*it)->alive()) {
            (*it)->send(rtpPacket);
        }
    }
}

bool MediaSession::startMulticast() {
    // 随机生成多播地址
    sockaddr_in addr{};
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand() % range));
    mMulticastAddr = inet_ntoa(addr.sin_addr);

    int rtpSockfd1, rtcpSockfd1;
    int rtpSockfd2, rtcpSockfd2;
    uint16_t rtpPort1, rtcpPort1;
    uint16_t rtpPort2, rtcpPort2;

    bool ret;

    rtpSockfd1 = sockets::createUdpSocket();
    assert(rtpSockfd1 > 0);

    rtpSockfd2 = sockets::createUdpSocket();
    assert(rtpSockfd2 > 0);

    rtcpSockfd1 = sockets::createUdpSocket();
    assert(rtcpSockfd1 > 0);

    rtcpSockfd2 = sockets::createUdpSocket();
    assert(rtcpSockfd2);

    uint16_t port = rand() & 0xFFFE; // 65535

    if (port < 10000) {
        port += 10000;
    }

    rtpPort1 = port;
    rtcpPort1 = port + 1;
    rtpPort2 = rtcpPort1 + 1;
    rtcpPort2 = rtpPort2 + 1;

    mMulticastRtpInstances[TRACK_ID_0] =
        RtpInstance::createNewOverUdp(rtpSockfd1, 0, mMulticastAddr, rtpPort1);
    mMulticastRtpInstances[TRACK_ID_1] =
        RtpInstance::createNewOverUdp(rtpSockfd2, 0, mMulticastAddr, rtpPort2);
    mMulticastRtcpInstances[TRACK_ID_0] =
        RtcpInstance::createNew(rtcpSockfd1, 0, mMulticastAddr, rtcpPort1);
    mMulticastRtcpInstances[TRACK_ID_1] =
        RtcpInstance::createNew(rtcpSockfd2, 0, mMulticastAddr, rtcpPort2);

    this->addRtpInstance(TRACK_ID_0, mMulticastRtpInstances[TRACK_ID_0]);
    this->addRtpInstance(TRACK_ID_1, mMulticastRtpInstances[TRACK_ID_1]);

    mMulticastRtpInstances[TRACK_ID_0]->setAlive(true);
    mMulticastRtpInstances[TRACK_ID_1]->setAlive(true);

    mIsStartMulticast = true;

    return true;
}

bool MediaSession::isStartMulticast() {
    return mIsStartMulticast;
}

uint16_t MediaSession::getMulticastDestRtpPort(MediaSession::TrackId trackId) {
    if (trackId > TRACK_ID_1 || !mMulticastRtpInstances[trackId]) {
        return -1;
    }

    return mMulticastRtpInstances[trackId]->getPeerPort();
}
