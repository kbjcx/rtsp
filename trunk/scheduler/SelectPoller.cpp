#include <bits/types/struct_timeval.h>
#include <sys/select.h>
#include <utility>

#include "../base/log.h"
#include "Event.h"
#include "SelectPoller.h"

SelectPoller::SelectPoller() {
    FD_ZERO(&mReadSet);
    FD_ZERO(&mWriteSet);
    FD_ZERO(&mErrorSet);
    LOGINFO("SelectPoller()");
}

SelectPoller::~SelectPoller() {
    LOGINFO("~SelectPoller()");
}

SelectPoller* SelectPoller::createNew() {
    return new SelectPoller();
}

bool SelectPoller::addIOEvent(IOEvent* event) {
    return updateIOEvent(event);
}

bool SelectPoller::updateIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        LOGERROR("fd = %d", fd);
        return false;
    }

    FD_CLR(fd, &mReadSet);
    FD_CLR(fd, &mWriteSet);
    FD_CLR(fd, &mErrorSet);

    auto it = mEventMap.find(fd);
    if (it == mEventMap.end()) {
        if (event->isReadHandling()) {
            FD_SET(fd, &mReadSet);
        }
        if (event->isWriteHandling()) {
            FD_SET(fd, &mWriteSet);
        }
        if (event->isErrorHandling()) {
            FD_SET(fd, &mErrorSet);
        }

        mEventMap.insert(std::make_pair(fd, event));
    } else {
        if (event->isReadHandling()) {
            FD_SET(fd, &mReadSet);
        }
        if (event->isWriteHandling()) {
            FD_SET(fd, &mWriteSet);
        }
        if (event->isErrorHandling()) {
            FD_SET(fd, &mErrorSet);
        }
    }

    if (mEventMap.empty()) {
        mMaxNumSockets = 0;
    } else {
        mMaxNumSockets = mEventMap.rbegin()->first + 1;
    }

    return true;
}

bool SelectPoller::removeIOEvent(IOEvent* event) {
    int fd = event->getFd();
    if (fd < 0) {
        return false;
    }

    FD_CLR(fd, &mReadSet);
    FD_CLR(fd, &mWriteSet);
    FD_CLR(fd, &mErrorSet);

    auto it = mEventMap.find(fd);

    if (it != mEventMap.end()) {
        mEventMap.erase(it);
    }

    if (mEventMap.empty()) {
        mMaxNumSockets = 0;
    } else {
        mMaxNumSockets = mEventMap.rbegin()->first + 1;
    }

    return true;
}

void SelectPoller::handleEvent() {
    fd_set readSet = mReadSet;
    fd_set writeSet = mWriteSet;
    fd_set errorSet = mErrorSet;
    timeval timeout;
    int ret, rEvent;

    timeout.tv_sec = 1000;
    timeout.tv_usec = 0;

    ret = select(mMaxNumSockets, &mReadSet, &mWriteSet, &mErrorSet, &timeout);

    if (ret < 0) {
        return;
    } else {
        LOGINFO("find active fd, ret = %d\n", ret);
    }

    for (auto it = mEventMap.begin(); it != mEventMap.end(); ++it) {
        rEvent = 0;
        if (FD_ISSET(it->first, &readSet)) {
            rEvent |= IOEvent::EVENT_READ;
        }

        if (FD_ISSET(it->first, &writeSet)) {
            rEvent |= IOEvent::EVENT_WRITE;
        }

        if (FD_ISSET(it->first, &errorSet)) {
            rEvent |= IOEvent::EVENT_ERROR;
        }

        if (rEvent != 0) {
            it->second->setReEvent(rEvent);
            mIOEvents.push_back(it->second);
        }
    }

    for (auto& ioEvent : mIOEvents) {
        ioEvent->handleEvent();
    }

    mIOEvents.clear();
}
