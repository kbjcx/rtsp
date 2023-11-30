#ifndef RTSPSERVER_TIMER_H
#define RTSPSERVER_TIMER_H

#include <cstdint>
#include <map>

#include <stdint.h>

class EventScheduler;
class Poller;
class TimerEvent;
class IOEvent;

class Timer {
public:
    using TimerId = uint32_t;
    using Timestamp = int64_t;
    using TimeInterval = uint32_t;

    ~Timer();

    static Timestamp getCurTime();
    static Timestamp getCurTimestamp();

private:
    friend class TimerManager;
    Timer(TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval,
        TimerId timerId);

private:
    bool handleEvent();

private:
    TimerEvent* mTimerEvent;
    Timestamp mTimestamp;
    TimeInterval mTimeInterval;
    TimerId mTimerId;

    bool mRepeat;
};

class TimerManager {
public:
    static TimerManager* createNew(EventScheduler* scheduler);

    TimerManager(EventScheduler* scheduler);
    ~TimerManager();

    Timer::TimerId addTimer(
        TimerEvent* event, Timer::Timestamp timestamp, Timer::TimeInterval timeInterval);
    bool removeTimer(Timer::TimerId timerId);

private:
    static void readCallback(void* arg);
    void handleRead();
    void modifyTimeout();

private:
    Poller* mPoller;
    std::map<Timer::TimerId, Timer> mTimers;
    std::multimap<Timer::Timestamp, Timer> mEvents;
    uint32_t mLastTimerId;
    int mTimerFd;
    IOEvent* mTimerIOEvent;
};

#endif
