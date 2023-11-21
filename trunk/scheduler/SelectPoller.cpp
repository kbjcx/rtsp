#include "SelectPoller.h"

#include <sys/select.h>

#include <utility>

#include "../base/log.h"
#include "Event.h"

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
}
