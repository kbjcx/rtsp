#ifndef RTSPSERVER_H264SINK_H
#define RTSPSERVER_H264SINK_H

#include <stdint.h>

#include "MediaSource.h"
#include "Rtp.h"
#include "Sink.h"

class H264Sink : public Sink {
public:
    static H264Sink* createNew(UsageEnvironment* env, MediaSource* mediaSource);

    H264Sink(UsageEnvironment* env, MediaSource* mediaSource);

    virtual ~H264Sink();

    std::string getMediaDescription(uint16_t port) override;
    void sendFrame(MediaFrame* frame) override;
    std::string getAttribute() override;

private:
    RtpPacket mRtpPacket;
    int mClockRate;
    int mFps;
};

#endif
