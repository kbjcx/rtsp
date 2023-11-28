#ifndef RTSPSERVER_MEDIASESSION_H
#define RTSPSERVER_MEDIASESSION_H

#include <Rtp.h>
#include <cstdint>
#include <list>
#include <string>

#include "RtpInstance.h"
#include "Sink.h"

#define MEDIA_MAX_TRACK_NUM 2

class MediaSession {
public:
    enum TrackId {
        TRACK_ID_NONE = -1,
        TRACK_ID_0 = 0,
        TRACK_ID_1 = 1
    };

    static MediaSession* createNew(std::string sessionName);
    explicit MediaSession(const std::string& sessionName);
    ~MediaSession();

    std::string name() const {
        return mSessionName;
    }

    std::string generateSPDDescription();

    bool addSink(TrackId trackId, Sink* sink); // 添加数据生产者

    // 添加数据消费者
    bool addRtpInstance(TrackId trackId, RtpInstance* rtpInstance);
    // 删除数据消费者
    bool removeRtpInstance(RtpInstance* rtpInstance);

    bool startMulticast();

    bool isStartMulticast();

    std::string getMulticastDestAddr() const {
        return mMulticastAddr;
    }

    uint16_t getMulticastDestRtpPort(TrackId trackId);

private:
    class Track {
    public:
        Sink* mSink;
        int mTrackId;
        bool mIsAlive;
        std::list<RtpInstance*> mRtpInstances;
    };

    Track* getTrack(TrackId trackId);

    static void sendPacketCallback(
        void* arg1, void* arg2, void* packet, Sink::PacketType packetType);

    void handleSendRtpPacket(Track* track, RtpPacket* rtpPacket);

private:
    std::string mSessionName;
    std::string mSdp;
    Track mTracks[MEDIA_MAX_TRACK_NUM];
    bool mIsStartMulticast;
    std::string mMulticastAddr;
    RtpInstance* mMulticastRtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* mMulticastRtcpInstances[MEDIA_MAX_TRACK_NUM];
};

#endif
