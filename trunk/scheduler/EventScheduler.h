#ifndef RTSPSERVER_EVENTSCHEDULER_H
#define RTSPSERVER_EVENTSCHEDULER_H

#include <mutex>
#include <queue>

#include <stdint.h>

#include <vector>

#include "Event.h"
#include "Timer.h"

class Poller;

class EventScheduler {
public:
    enum PollerType {
        POLLER_SELECT,
        POLLER_POLL,
        POLLER_EPOLL
    };

    static EventScheduler* createNew(PollerType type);

    explicit EventScheduler(PollerType type);
    virtual ~EventScheduler();

public:
    bool addTriggerEvent(TriggerEvent* event);
    Timer::TimerId addTimerEventRunAfter(TimerEvent* event, Timer::TimeInterval delay);
    Timer::TimerId addTimerEventRunAt(TimerEvent* event, Timer::Timestamp when);
    Timer::TimerId addTimerEventRunEvery(TimerEvent* event, Timer::TimeInterval interval);
    bool removeTimerEvent(Timer::TimerId timerId);

    bool addIOEvent(IOEvent* event);
    bool updateIOEvent(IOEvent* event);
    bool removeIOEvent(IOEvent* event);

    void loop();
    Poller* poller();
    void setTimerManagerReadCalback(EventCallback cb, void* arg);

private:
    void handleTriggerEvents();

private:
    bool mQuit;
    Poller* mPoller;
    TimerManager* mTimerManager;
    std::vector<TriggerEvent*> mTriggerEvents;
    std::mutex mMutex;
    // win系统专用调用
    EventCallback mTimerManagerReadCallback;
    void* mTimerManagerArg;
};

#endif
