#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "../base/log.h"
#include "AACSink.h"
#include "MediaSource.h"
#include "Rtp.h"
#include "Sink.h"

AACSink* AACSink::createNew(UsageEnvironment* env, MediaSource* mediaSource) {
    return new AACSink(env, mediaSource);
}

AACSink::AACSink(UsageEnvironment* env, MediaSource* mediaSource)
    : Sink(env, mediaSource, RTP_PAYLOAD_TYPE_AAC)
    , mSampleRate(44100)
    , mChannels(2)
    , mFps(mediaSource->getFps()) {
    LOGINFO("AACSink()");
    mMarker = 1;
    runEvery(1000 / mFps);
}

AACSink::~AACSink() {
    LOGINFO("~AACSink()");
}

std::string AACSink::getMediaDescription(uint16_t port) {
    char buf[100];
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, mPayloadType);
    return buf;
}

// clang-format off
static uint32_t AACSampleRate[16] = {
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};
// clang-format on

std::string AACSink::getAttribute() {
    char buf[500];
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", mSampleRate, mChannels);

    uint8_t index = 0;
    for (index = 0; index < 16; ++index) {
        if (AACSampleRate[index] == mSampleRate) {
            break;
        }
    }

    if (index == 16) {
        return "";
    }

    uint8_t profile = 1;
    char configStr[10] = {0};
    sprintf(configStr, "%02x%02x", (uint8_t)((profile + 1) << 3) | (index >> 1),
        (uint8_t)((index << 7) | (mChannels << 3)));
    sprintf(buf + strlen(buf),
        "a=fmtp:%d profile-level-id=1;"
        "mode=AAC-hbr"
        "sizelength=13;indexlength=3;indexdeltalength=3;"
        "config=%04u",
        mPayloadType, atoi(configStr));

    return buf;
}

void AACSink::sendFrame(MediaFrame* frame) {
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeader;
    int frameSize = frame->mSize - 7; // 去掉AAC头部

    rtpHeader->payload[0] = 0x00;
    rtpHeader->payload[1] = 0x10;
    rtpHeader->payload[2] = (frameSize & 0x1FE0) >> 5;
    rtpHeader->payload[3] = (frameSize & 0x1F) << 3;

    memcpy(rtpHeader->payload + 4, frame->mBuf + 7, frameSize);
    mRtpPacket.mSize = RTP_HEADER_SIZE + 4 + frameSize;

    sendRtpPacket(&mRtpPacket);

    ++mSeq;

    mTimestamp += mSampleRate * (1000 / mFps) / 1000;
}
