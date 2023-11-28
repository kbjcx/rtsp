#include "H264Sink.h"

#include <MediaSource.h>
#include <Rtp.h>
#include <Sink.h>
#include <stdio.h>
#include <string.h>
#include <cstdint>

#include "../base/log.h"

H264Sink* H264Sink::createNew(UsageEnvironment* env, MediaSource* mediaSource) {
    if (!mediaSource) {
        return nullptr;
    }
    return new H264Sink(env, mediaSource);
}

H264Sink::H264Sink(UsageEnvironment* env, MediaSource* mediaSource)
    : Sink(env, mediaSource, RTP_PAYLOAD_TYPE_H264)
    , mClockRate(90000)
    , mFps(mediaSource->getFps()) {
    LOGINFO("H264Sink()");
    runEvery(1000 / mFps);
}

H264Sink::~H264Sink() {
    LOGINFO("~H264Sink()");
}

std::string H264Sink::getMediaDescription(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP %d", port, mPayloadType);

    return std::string(buf);
}

std::string H264Sink::getAttribute() {
    char buf[100];
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", mPayloadType, mClockRate);
    sprintf(buf + strlen(buf), "a=framerate:%d", mFps);

    return buf;
}

void H264Sink::sendFrame(MediaFrame* frame) {
    // 发送RTP数据包
    RtpHeader* rtpHeader = mRtpPacket.mRtpHeader;
    uint8_t naluType = frame->mBuf[0];

    if (frame->mSize <= RTP_MAX_PACKET_SIZE) {
        memcpy(rtpHeader->payload, frame->mBuf, frame->mSize);
        mRtpPacket.mSize = RTP_HEADER_SIZE + frame->mSize;
        sendRtpPacket(&mRtpPacket);
        ++mSeq;
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) { // SPS and PPS
            return;
        }
    } else {
        int pktNum = frame->mSize / RTP_MAX_PACKET_SIZE;
        int remainPktSize = frame->mSize % RTP_MAX_PACKET_SIZE;

        int i, pos = 1;

        for (i = 0; i < pktNum; ++i) {
            rtpHeader->payload[0] = (naluType & 0x60) | 28;

            rtpHeader->payload[1] = naluType & 0x1F;

            if (i == 0) {
                rtpHeader->payload[1] |= 0x80; // start
            } else if (remainPktSize == 0 && i == pktNum - 1) {
                rtpHeader->payload[1] |= 0x40; // end
            }

            memcpy(rtpHeader->payload + 2, frame->mBuf + pos, RTP_MAX_PACKET_SIZE);
            mRtpPacket.mSize = RTP_HEADER_SIZE + 2 + RTP_MAX_PACKET_SIZE;
            sendRtpPacket(&mRtpPacket);

            mSeq++;
            pos += RTP_MAX_PACKET_SIZE;
        }

        if (remainPktSize > 0) {
            rtpHeader->payload[0] = (naluType & 0x60) | 28;
            rtpHeader->payload[1] = (naluType & 0x1F) | 0x40;

            memcpy(rtpHeader->payload + 2, frame->mBuf + pos, remainPktSize);
            mRtpPacket.mSize = RTP_HEADER_SIZE + 2 + remainPktSize;
            sendRtpPacket(&mRtpPacket);

            ++mSeq;
        }
    }

    mTimestamp += mClockRate / mFps;
}
