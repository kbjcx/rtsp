#include <netinet/in.h>
#include <cstdlib>

#include "../base/log.h"
#include "../scheduler/SocketsOptions.h"
#include "MediaSource.h"
#include "Rtp.h"
#include "Sink.h"

Sink::Sink(UsageEnvironment* env, MediaSource* mediaSource, int payloadType)
    : mMediaSource(mediaSource)
    , mEnv(env)
    , mCsrcLen(0)
    , mExtension(0)
    , mPadding(0)
    , mVersion(RTP_VERSION)
    , mPayloadType(payloadType)
    , mMarker(0)
    , mSeq(0)
    , mSsrc(rand())
    , mTimestamp(0)
    , mTimerId(0)
    , mSessionSendPacketCallback(nullptr)
    , mArg1(nullptr)
    , mArg2(nullptr) {
    LOGINFO("Sink()");
    mTimerEvent = TimerEvent::createNew(this);
    mTimerEvent->setTimeoutCallback(cbTimeout);
}

Sink::~Sink() {
    LOGINFO("~Sink()");
    delete mTimerEvent;
    delete mMediaSource;
}

void Sink::stopTimerEvent() {
    mTimerEvent->stop();
}

void Sink::setSessionCallback(SessionSendPacketCallback cb, void* arg1, void* arg2) {
    mSessionSendPacketCallback = cb;
    mArg1 = arg1;
    mArg2 = arg2;
}

void Sink::sendRtpPacket(RtpPacket* packet) {
    auto rtpHeader = packet->mRtpHeader;
    rtpHeader->csrcLen = mCsrcLen;
    rtpHeader->extension = mExtension;
    rtpHeader->padding = mPadding;
    rtpHeader->version = mVersion;
    rtpHeader->payloadType = mPayloadType;
    rtpHeader->marker = mMarker;
    rtpHeader->seq = htons(mSeq);
    rtpHeader->timestamp = htonl(mTimestamp);
    rtpHeader->ssrc = htonl(mSsrc);

    if (mSessionSendPacketCallback) {
        // arg1 mediaSession 对象指针
        // arg2 mediaSession 被回调track对象指针
        mSessionSendPacketCallback(mArg1, mArg2, packet, PacketType::RTP_PACKET);
    }
}

void Sink::cbTimeout(void* arg) {
    auto sink = (Sink*)arg;
    sink->handleTimeout();
}

void Sink::handleTimeout() {
    auto frame = mMediaSource->getFrameFromOutputQueue();
    if (!frame) {
        return;
    }

    this->sendFrame(frame); // 由具体的子类实现
    mMediaSource->putFrameToInputQueue(frame);
}

void Sink::runEvery(Timer::TimeInterval interval) {
    mTimerId = mEnv->scheduler()->addTimerEventRunEvery(mTimerEvent, interval);
}
