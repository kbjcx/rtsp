#ifndef RTSPSERVER_H264MEDIASOURCE_H
#define RTSPSERVER_H264MEDIASOURCE_H

#include <cstdint>
#include <cstdio>
#include <string>

#include "MediaSource.h"

class H264MediaSource : public MediaSource {
public:
    static H264MediaSource* createNew(UsageEnvironment* env, const std::string& file);

    H264MediaSource(UsageEnvironment* env, const std::string& file);
    virtual ~H264MediaSource();

protected:
    void handleTask() override;

private:
    int getFrameFromH264File(uint8_t* frame, int size);

private:
    FILE* mFile;
};

#endif
