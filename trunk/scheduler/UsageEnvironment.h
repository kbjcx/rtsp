#ifndef RTSPSERVER_USAGEENVIRONMENT
#define RTSPSERVER_USAGEENVIRONMENT

#include "EventScheduler.h"
#include "Thread.h"
#include "ThreadPool.h"

class UsageEnvironment {
public:
    static UsageEnvironment* createNew(EventScheduler* scheduler, ThreadPool* threadPool);

    UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadPool);
    ~UsageEnvironment();

    EventScheduler* scheduler();
    ThreadPool* threadPool();

private:
    EventScheduler* mEventScheduler;
    ThreadPool* mThreadPool;
};

#endif
