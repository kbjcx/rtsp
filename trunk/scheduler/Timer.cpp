#include "Timer.h"

#include <bits/types/struct_itimerspec.h>
#include <bits/types/struct_timespec.h>

#include <chrono>
#include <ctime>
#include <map>

#include <sys/timerfd.h>
#include <time.h>

#include <utility>

#include "../base/log.h"
#include "Event.h"
#include "EventScheduler.h"
#include "Poller.h"

static bool timerFdSetTime(int fd, Timer::Timestamp when, Timer::TimeInterval period) {
    itimerspec newVal;
    newVal.it_value.tv_sec = when / 1000;
    newVal.it_value.tv_nsec = when % 1000 * 1000 * 1000;
    newVal.it_interval.tv_sec = period / 1000;
    newVal.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    int oldValue = timerfd_settime(fd, TFD_TIMER_ABSTIME, &newVal, nullptr);
    if (oldValue < 0) {
        return false;
    }

    return true;
}

Timer::Timer(
    TimerEvent* event, Timestamp timestamp, TimeInterval timeInterval, TimerId timerId)
    : mTimerEvent(event)
    , mTimestamp(timestamp)
    , mTimeInterval(timeInterval)
    , mTimerId(timerId) {
    if (timeInterval > 0) {
        mRepeat = true;
    } else {
        mRepeat = false;
    }
}

Timer::~Timer() = default;

Timer::Timestamp Timer::getCurTime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

Timer::Timestamp Timer::getCurTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
}

bool Timer::handleEvent() {
    if (!mTimerEvent) {
        return false;
    }

    return mTimerEvent->handleEvent();
}

TimerManager* TimerManager::createNew(EventScheduler* scheduler) {
    if (!scheduler) {
        return nullptr;
    }

    return new TimerManager(scheduler);
}

TimerManager::TimerManager(EventScheduler* scheduler)
    : mPoller(scheduler->poller())
    , mLastTimerId(0) {
    mTimerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (mTimerFd < 0) {
        LOGERROR("create timerFd error");
        return;
    } else {
        LOGINFO("create timerFd = %d", mTimerFd);
    }

    mTimerIOEvent = IOEvent::createNew(mTimerFd, this);
    mTimerIOEvent->setReadCallback(readCallback);
    mTimerIOEvent->enableReadHandling();
    modifyTimeout();
    mPoller->addIOEvent(mTimerIOEvent);
}

TimerManager::~TimerManager() {
    mPoller->removeIOEvent(mTimerIOEvent);
    delete mTimerIOEvent;
}

Timer::TimerId TimerManager::addTimer(
    TimerEvent* event, Timer::Timestamp timestamp, Timer::TimeInterval timeInterval) {
    ++mLastTimerId;
    Timer timer(event, timestamp, timeInterval, mLastTimerId);
    mTimers.insert(std::make_pair(mLastTimerId, timer));
    mEvents.insert(std::make_pair(timestamp, timer));
    modifyTimeout();
    return mLastTimerId;
}

bool TimerManager::removeTimer(Timer::TimerId timerId) {
    std::map<Timer::TimerId, Timer>::iterator it = mTimers.find(timerId);
    if (it != mTimers.end()) {
        mTimers.erase(timerId);
        // 还需要删除mEvents的事件
    }

    modifyTimeout();

    return true;
}

/**
 * @brief 用来设置更新最近的超时时间
 *
 */
void TimerManager::modifyTimeout() {
    auto it = mEvents.begin();
    if (it != mEvents.end()) {
        Timer timer = it->second;
        timerFdSetTime(mTimerFd, timer.mTimestamp, timer.mTimeInterval);
    } else {
        timerFdSetTime(mTimerFd, 0, 0);
    }
}

void TimerManager::readCallback(void* arg) {
    TimerManager* timerManager = (TimerManager*)arg;
    timerManager->handleRead();
}

void TimerManager::handleRead() {
    Timer::Timestamp timestamp = Timer::getCurTime();
    if (!mTimers.empty() && !mEvents.empty()) {
        auto it = mEvents.begin();
        Timer timer = it->second;
        int expire = timer.mTimestamp - timestamp;

        if (expire <= 0) {
            // 已经过了超时时间，触发超时事件
            // TODO存在问题，只会触发multimap的第一个事件，应该出发multimap的同一key的所有时间
            bool timerEventIsStop = timer.handleEvent();
            mEvents.erase(it);
            if (timer.mRepeat) {
                if (timerEventIsStop) {
                    mTimers.erase(timer.mTimerId);
                } else {
                    timer.mTimestamp = timestamp + timer.mTimeInterval;
                    mEvents.insert(std::make_pair(timer.mTimestamp, timer));
                }
            }
        } else {
            mTimers.erase(timer.mTimerId);
        }
    }

    modifyTimeout();
}
