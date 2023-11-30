#include <fcntl.h>
#include <cstdint>
#include <cstdio>
#include <mutex>

#include "../base/log.h"
#include "H264MediaSource.h"

static inline int startCode3(uint8_t* buf);
static inline int startCode4(uint8_t* buf);

H264MediaSource* H264MediaSource::createNew(
    UsageEnvironment* env, const std::string& file) {
    return new H264MediaSource(env, file);
}

H264MediaSource::H264MediaSource(UsageEnvironment* env, const std::string& file)
    : MediaSource(env) {
    mSourceName = file;
    mFile = fopen(file.data(), "rb");
    setFps(25);

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mEnv->threadPool()->addTask(mTask);
    }
}

H264MediaSource::~H264MediaSource() {
    fclose(mFile);
}

void H264MediaSource::handleTask() {
    std::lock_guard<std::mutex> lock(mMutex);

    if (mFrameInputQueue.empty()) {
        return;
    }

    MediaFrame* frame = mFrameInputQueue.front();
    int startCode = 0;

    while (true) {
        frame->mSize = getFrameFromH264File(frame->temp, FRAME_MAX_SIZE);

        if (frame->mSize < 0) {
            return;
        }

        if (startCode3(frame->temp)) {
            startCode = 3;
        } else {
            startCode = 4;
        }

        frame->mBuf = frame->temp + startCode;
        frame->mSize -= startCode;

        uint8_t naluType = frame->mBuf[0] & 0x1F;

        if (naluType == 0x09) {
            continue;
        } else if (naluType == 0x07 || naluType == 0x08) {
            break;
        } else {
            break;
        }
    }

    mFrameInputQueue.pop();
    mFrameOutputQueue.push(frame);
}

static inline int startCode3(uint8_t* buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static inline int startCode4(uint8_t* buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t* findNextStartCode(uint8_t* buf, int len) {
    int i;
    if (len < 3) {
        return nullptr;
    }

    for (i = 0; i < len - 3; ++i) {
        if (startCode3(buf) || startCode4(buf)) {
            return buf;
        }
        ++buf;
    }

    if (startCode3(buf)) {
        return buf;
    }

    return nullptr;
}

int H264MediaSource::getFrameFromH264File(uint8_t* frame, int size) {
    if (mFile == nullptr) {
        return -1;
    }

    int r, frameSize;
    uint8_t* nextStartCode;

    r = fread(frame, 1, size, mFile);

    if (!startCode3(frame) && !startCode4(frame)) {
        fseek(mFile, 0, SEEK_SET);
        LOGERROR("Read %s error, no startCode3 and no startCode4", mSourceName.data());
        return -1;
    }

    nextStartCode = findNextStartCode(frame + 3, r - 3);

    if (!nextStartCode) {
        fseek(mFile, 0, SEEK_SET);
        frameSize = r;
        LOGERROR("Read %s error, no nextStartCode, r = %d", mSourceName.data(), r);
    } else {
        frameSize = (nextStartCode - frame);
        fseek(mFile, frameSize - r, SEEK_CUR);
    }

    return frameSize;
}
