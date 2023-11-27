#ifndef RTSPSERVER_MEDIASOURCE_H
#define RTSPSERVER_MEDIASOURCE_H

#include <stdint.h>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <queue>

#include "../scheduler/ThreadPool.h"
#include "../scheduler/UsageEnvironment.h"

#define FRAME_MAX_SIZE    (1024 * 200)
#define DEFAULT_FRAME_NUM 4

class MediaFrame {
public:
    MediaFrame()
        : temp((uint8_t*)malloc(FRAME_MAX_SIZE))
        , mBuf(nullptr)
        , mSize(0) {}
    ~MediaFrame() {
        delete temp;
    }

public:
    uint8_t* temp; // 容器
    uint8_t* mBuf; // 引用容器
    int mSize;
};

class MediaSource {
public:
    explicit MediaSource(UsageEnvironment* env);
    virtual ~MediaSource();

    MediaFrame* getFrameFromOutputQueue();        // 从输出队列获取帧
    void putFrameToInputQueue(MediaFrame* frame); // 把帧放入输出队列
    int getFps() const {
        return mFps;
    }
    std::string getSourceName() {
        return mSourceName;
    }

private:
    static void taskCallback(void* arg);

protected:
    virtual void handleTask() = 0;
    void setFps(int fps) {
        mFps = fps;
    }

protected:
    UsageEnvironment* mEnv;
    MediaFrame mFrames[DEFAULT_FRAME_NUM];
    std::queue<MediaFrame*> mFrameInputQueue;
    std::queue<MediaFrame*> mFrameOutputQueue;

    std::mutex mMutex;
    ThreadPool::Task mTask;
    int mFps;
    std::string mSourceName;
};

#endif
