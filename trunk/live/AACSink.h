#ifndef RTSPSERVER_AACSINK_H
#define RTSPSERVER_AACSINK_H

#include <cstdint>

#include "../scheduler/UsageEnvironment.h"
#include "MediaSource.h"
#include "Rtp.h"
#include "Sink.h"

class AACSink : public Sink {
public:
    static AACSink* createNew(UsageEnvironment* env, MediaSource* mediaSource);

    AACSink(UsageEnvironment* env, MediaSource* mediaSource);

    virtual ~AACSink();

    std::string getMediaDescription(uint16_t port) override;
    std::string getAttribute() override;

protected:
    void sendFrame(MediaFrame* frame) override;

private:
    RtpPacket mRtpPacket;
    uint32_t mSampleRate;
    uint32_t mChannels;
    int mFps;
};

#endif
