#ifndef RTSPSERVER_SELECTPOLLER_H
#define RTSPSERVER_SELECTPOLLER_H

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include "Event.h"
#include "Poller.h"

class SelectPoller : public Poller {
public:
    SelectPoller();
    ~SelectPoller();

    static SelectPoller* createNew();

    bool addIOEvent(IOEvent* event) override;
    bool updateIOEvent(IOEvent* event) override;
    bool removeIOEvent(IOEvent* event) override;
    void handleEvent() override;

private:
    fd_set mReadSet;
    fd_set mWriteSet;
    fd_set mErrorSet;
    int mMaxNumSockets;
    std::vector<IOEvent*> mIOEvents;
};

#endif
