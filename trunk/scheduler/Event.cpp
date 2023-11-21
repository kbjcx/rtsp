#include "Event.h"

#include <stdio.h>

#include "../base/log.h"

TriggerEvent* TriggerEvent::createNew(void* arg) {
    return new TriggerEvent(arg);
}

TriggerEvent* TriggerEvent::createNew() {
    return new TriggerEvent(nullptr);
}

TriggerEvent::TriggerEvent(void* arg)
    : mArg(arg)
    , mTriggerCallback(nullptr) {
    LOGINFO("TriggerEvent()");
}

TriggerEvent::~TriggerEvent() {
    LOGINFO("~TriggerEvent()");
}

void TriggerEvent::handleEvent() {
    if (mTriggerCallback) {
        mTriggerCallback(mArg);
    }
}

TimerEvent* TimerEvent::createNew(void* arg) {
    return new TimerEvent(arg);
}

TimerEvent* TimerEvent::createNew() {
    return new TimerEvent(nullptr);
}

TimerEvent::TimerEvent(void* arg)
    : mArg(arg)
    , mTimeoutCallback(nullptr)
    , mIsStop(false) {
    LOGINFO("TimeEvent()");
}

TimerEvent::~TimerEvent() {
    LOGINFO("~TimeEvent()");
}

bool TimerEvent::handleEvent() {
    if (mIsStop) {
        return mIsStop;
    }

    if (mTimeoutCallback) {
        mTimeoutCallback(mArg);
    }

    return mIsStop;
}

void TimerEvent::stop() {
    mIsStop = true;
}

IOEvent* IOEvent::createNew(int fd, void* arg) {
    if (fd < 0) {
        return nullptr;
    }

    return new IOEvent(fd, arg);
}

IOEvent* IOEvent::createNew(int fd) {
    if (fd < 0) {
        return nullptr;
    }

    return new IOEvent(fd, nullptr);
}

IOEvent::IOEvent(int fd, void* arg)
    : mFd(fd)
    , mArg(arg)
    , mEvent(EVENT_NONE)
    , mReEvent(EVENT_NONE)
    , mReadCallback(nullptr)
    , mWriteCallback(nullptr)
    , mErrorCallback(nullptr) {
    LOGINFO("IOEvent() fd = %d", mFd);
}

IOEvent::~IOEvent() {
    LOGINFO("~IOEvent() fd = %d", mFd);
}

void IOEvent::handleEvent() {
    if (mReadCallback && (mReEvent & EVENT_READ) != 0) {
        mReadCallback(mArg);
    }

    if (mWriteCallback && (mReEvent & EVENT_WRITE) != 0) {
        mWriteCallback(mArg);
    }

    if (mErrorCallback && (mReEvent & EVENT_ERROR) != 0) {
        mErrorCallback(mArg);
    }
}
