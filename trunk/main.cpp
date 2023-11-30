#include <cmath>
#include <cstdlib>
#include <ctime>

#include "base/log.h"
#include "live/AACMediaSource.h"
#include "live/AACSink.h"
#include "live/H264MediaSource.h"
#include "live/H264Sink.h"
#include "live/InetAddress.h"
#include "live/MediaSessionManager.h"
#include "live/MediaSource.h"
#include "live/RtspServer.h"
#include "live/Sink.h"
#include "scheduler/EventScheduler.h"
#include "scheduler/ThreadPool.h"
#include "scheduler/UsageEnvironment.h"

int main(int argc, char* argv[]) {
    srand(time(nullptr));

    EventScheduler* scheduler = EventScheduler::createNew(EventScheduler::POLLER_SELECT);
    ThreadPool* threadpool = ThreadPool::createNew(1);
    MediaSessionManager* sessionManager = MediaSessionManager::createNew();
    UsageEnvironment* env = UsageEnvironment::createNew(scheduler, threadpool);

    Ipv4Address rtspAddr("0.0.0.0", 8554);
    RtspServer* rtspServer = RtspServer::createNew(env, sessionManager, rtspAddr);

    LOGINFO("-----------session init start ---------------");
    {
        MediaSession* session = MediaSession::createNew("test");
        MediaSource* source = H264MediaSource::createNew(env, "../data/daliu.h264");
        Sink* sink = H264Sink::createNew(env, source);
        session->addSink(MediaSession::TRACK_ID_0, sink);

        source = AACMediaSource::createNew(env, "../data/daliu.aac");
        sink = AACSink::createNew(env, source);
        session->addSink(MediaSession::TRACK_ID_1, sink);

        sessionManager->addSession(session);
    }

    LOGINFO("----------session init end-------------------");

    rtspServer->start();

    env->scheduler()->loop();

    return 0;
}
