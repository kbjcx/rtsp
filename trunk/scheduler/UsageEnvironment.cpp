#include "UsageEnvironment.h"

#include "EventScheduler.h"
#include "ThreadPool.h"

UsageEnvironment* UsageEnvironment::createNew(
    EventScheduler* scheduler, ThreadPool* threadPool) {
    return new UsageEnvironment(scheduler, threadPool);
}

UsageEnvironment::UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadPool)
    : mEventScheduler(scheduler)
    , mThreadPool(threadPool) {}

UsageEnvironment::~UsageEnvironment() {}

EventScheduler* UsageEnvironment::scheduler() {
    return mEventScheduler;
}

ThreadPool* UsageEnvironment::threadPool() {
    return mThreadPool;
}
