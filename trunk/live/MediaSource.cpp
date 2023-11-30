#include "MediaSource.h"

#include <mutex>

#include "../base/log.h"

MediaSource::MediaSource(UsageEnvironment* env)
    : mEnv(env)
    , mFps(0) {
    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        mFrameInputQueue.push(&mFrames[i]);
    }

    mTask.setTaskCallback(taskCallback, this);
}

MediaSource::~MediaSource() {
    LOGINFO("~MediaSource()");
}

MediaFrame* MediaSource::getFrameFromOutputQueue() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mFrameOutputQueue.empty()) {
        return nullptr;
    }

    MediaFrame* frame = mFrameOutputQueue.front();
    mFrameOutputQueue.pop();

    return frame;
}

void MediaSource::putFrameToInputQueue(MediaFrame* frame) {
    std::lock_guard<std::mutex> lock(mMutex);
    mFrameInputQueue.push(frame);
    mEnv->threadPool()->addTask(mTask);
}

void MediaSource::taskCallback(void* arg) {
    auto mediaSource = (MediaSource*)arg;
    mediaSource->handleTask();
}
