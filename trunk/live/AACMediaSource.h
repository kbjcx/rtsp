#ifndef RTSPSERVER_AACMEDIASOURCE_H
#define RTSPSERVER_AACMEDIASOURCE_H

#include <cstdint>
#include <string>

#include "MediaSource.h"

class AACMediaSource : public MediaSource {
public:
    static AACMediaSource* createNew(UsageEnvironment* env, const std::string& file);

    AACMediaSource(UsageEnvironment* env, const std::string& file);
    virtual ~AACMediaSource();

protected:
    void handleTask() override;

private:
    struct AdtsHeader {
        unsigned int syncword;          // 同步字 12bit
        unsigned int id;                // 1bit MPEG标识符 0: MPEG-4, 1: MPEG-2
        unsigned int layer;             // 2bit 总是'00'
        unsigned int protectionAbsent;  // 1bit 1表示没有crc 0表示有crc
        unsigned int profile;           // 1bit 表示使用哪个级别的AAC
        unsigned int samplingFreqIndex; // 4bit 表示采样频率
        unsigned int privateBit;        // 1bit
        unsigned int channelCfg;        // 3bit 表示声道数
        unsigned int originalCopy;      // 1bit
        unsigned int home;              // 1bit
        unsigned int copyrightIdentificationBit;   // 1bit
        unsigned int copyrightIdentificationStart; // 1bit
        unsigned int aacFrameLength;               // 13bit 帧头 + AAC流长度
        unsigned int adtsBufferFullness; // 11bit 0x7FF 说明是码率可变的码流

        unsigned int numberOfRawDataBlockInFrame; // 2bit
    };

    bool parseAdtsHeader(uint8_t* in, AdtsHeader* res);

    int getFrameFromAACFile(uint8_t* buf, int size);

private:
    FILE* mFile;
    AdtsHeader mAdtsHeader;
};

#endif
