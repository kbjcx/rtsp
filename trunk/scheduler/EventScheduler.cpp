#include "EventScheduler.h"

#include <unistd.h>

#include <vector>

#include "../base/log.h"
#include "Event.h"
#include "Poller.h"
#include "SelectPoller.h"
#include "SocketsOptions.h"
#include "Timer.h"

#ifndef WIN32
#include <sys/eventfd.h>
#endif

EventScheduler* EventScheduler::createNew(PollerType type) {
    if (type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL) {
        return nullptr;
    }

    return new EventScheduler(type);
}

EventScheduler::EventScheduler(PollerType type)
    : mQuit(false) {
    switch (type) {
    case POLLER_SELECT:
        mPoller = SelectPoller::createNew();
        break;
    case POLLER_POLL:
        break;
    case POLLER_EPOLL:
        break;
    default:
        _exit(-1);
        break;
    }

    mTimerManager = TimerManager::createNew(this);
}

EventScheduler::~EventScheduler() {
    delete mTimerManager;
    delete mPoller;
}

bool EventScheduler::addTriggerEvent(TriggerEvent* event) {
    mTriggerEvents.push_back(event);
    return true;
}

Timer::TimerId EventScheduler::addTimerEventRunAfter(
    TimerEvent* event, Timer::TimeInterval delay) {
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += delay;
    return mTimerManager->addTimer(event, timestamp, 0);
}

Timer::TimerId EventScheduler::addTimerEventRunAt(
    TimerEvent* event, Timer::Timestamp when) {
    return mTimerManager->addTimer(event, when, 0);
}

Timer::TimerId EventScheduler::addTimerEventRunEvery(
    TimerEvent* event, Timer::TimeInterval interval) {
    Timer::Timestamp timestamp = Timer::getCurTime();
    timestamp += interval;
    return mTimerManager->addTimer(event, timestamp, interval);
}

bool EventScheduler::removeTimerEvent(Timer::TimerId timerId) {
    return mTimerManager->removeTimer(timerId);
}

bool EventScheduler::addIOEvent(IOEvent* event) {
    return mPoller->addIOEvent(event);
}

bool EventScheduler::updateIOEvent(IOEvent* event) {
    return mPoller->updateIOEvent(event);
}

bool EventScheduler::removeIOEvent(IOEvent* event) {
    return mPoller->removeIOEvent(event);
}

void EventScheduler::loop() {
    while (!mQuit) {
        handleTriggerEvents();
        mPoller->handleEvent();
    }
}

void EventScheduler::handleTriggerEvents() {
    if (!mTriggerEvents.empty()) {
        for (std::vector<TriggerEvent*>::iterator it = mTriggerEvents.begin();
             it != mTriggerEvents.end(); ++it) {
            (*it)->handleEvent();
        }

        mTriggerEvents.clear();
    }
}

Poller* EventScheduler::poller() {
    return mPoller;
}

void EventScheduler::setTimerManagerReadCalback(EventCallback cb, void* arg) {
    mTimerManagerReadCallback = cb;
    mTimerManagerArg = arg;
}
