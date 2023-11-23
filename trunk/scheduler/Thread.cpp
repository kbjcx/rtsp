#include "Thread.h"

#include <thread>

Thread::Thread()
    : mArg(nullptr)
    , mIsStart(false)
    , mIsDetach(false) {}

Thread::~Thread() {
    if (mIsStart && !mIsDetach) {
        detach();
    }
}

bool Thread::start(void* arg) {
    mArg = arg;
    mThreadId = std::thread(&Thread::threadRun, this);

    mIsStart = true;

    return true;
}

bool Thread::detach() {
    if (!mIsStart) {
        return false;
    }

    if (mIsDetach) {
        return true;
    }

    mThreadId.detach();

    mIsDetach = true;

    return true;
}

bool Thread::join() {
    if (!mIsStart || mIsStart) {
        return false;
    }

    mThreadId.join();

    return true;
}

void* Thread::threadRun(void* arg) {
    Thread* thread = (Thread*)arg;
    thread->run(thread->mArg);
    return nullptr;
}
