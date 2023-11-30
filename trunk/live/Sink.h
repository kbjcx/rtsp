#ifndef RTSPSERVER_SINK_H
#define RTSPSERVER_SINK_H

#include <stdint.h>
#include <cstdint>
#include <string>

#include "../scheduler/Event.h"
#include "../scheduler/UsageEnvironment.h"
#include "MediaSource.h"
#include "Rtp.h"

class Sink {
public:
    enum PacketType {
        UNKNOWN = -1,
        RTP_PACKET = 0
    };

    using SessionSendPacketCallback = void (*)(
        void* arg1, void* arg2, void* packet, PacketType packetType);

    Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType);

    virtual ~Sink();

    void stopTimerEvent();

    virtual std::string getMediaDescription(uint16_t port) = 0;
    virtual std::string getAttribute() = 0;

    void setSessionCallback(SessionSendPacketCallback cb, void* arg1, void* arg2);

protected:
    virtual void sendFrame(MediaFrame* frame) = 0;
    void sendRtpPacket(RtpPacket* packet);

    void runEvery(Timer::TimeInterval interval);

private:
    static void cbTimeout(void* arg);

    void handleTimeout();

protected:
    UsageEnvironment* mEnv;
    MediaSource* mMediaSource;
    SessionSendPacketCallback mSessionSendPacketCallback;
    void* mArg1;
    void* mArg2;

    uint8_t mCsrcLen;
    uint8_t mExtension;
    uint8_t mPadding;
    uint8_t mVersion;
    uint8_t mPayloadType;
    uint8_t mMarker;
    uint8_t mSeq;
    uint8_t mTimestamp;
    uint8_t mSsrc;

private:
    TimerEvent* mTimerEvent;
    Timer::TimerId mTimerId;
};

#endif
