#ifndef RTSPSERVER_MEDIASESSION_H
#define RTSPSERVER_MEDIASESSION_H

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

    std::string name() const;

    std::string generateSPDDescription();

    bool addSink(TrackId trackId, Sink* sink); // 添加数据生产者

    // 添加数据消费者
    bool addRtpInstance(TrackId trackId, RtpInstance* rtpInstance);
    // 删除数据消费者
    bool removeRtpInstance(RtpInstance* rtpInstance);

    bool startMulticasr();

    bool isStartMulticast();

    std::string getMulticastDestAddr() const;

    uint16_t getMulticastDestRtpPort(TrackId trackId);
};

#endif
