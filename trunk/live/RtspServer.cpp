#include <cassert>
#include <mutex>
#include <utility>

#include "../base/log.h"
#include "InetAddress.h"
#include "RtspConnection.h"
#include "RtspServer.h"

RtspServer* RtspServer::createNew(
    UsageEnvironment* env, MediaSessionManager* sessionManager, Ipv4Address& addr) {
    return new RtspServer(env, sessionManager, addr);
}

RtspServer::RtspServer(
    UsageEnvironment* env, MediaSessionManager* sessionManager, Ipv4Address& addr)
    : mEnv(env)
    , mSessionManager(sessionManager)
    , mAddr(addr)
    , mListen(false) {
    mFd = sockets::createTcpSocket();
    sockets::setReuseAddr(mFd, 1);

    if (!sockets::bind(mFd, mAddr.getIp(), mAddr.getPort())) {
        return;
    }

    LOGINFO("rtsp://%s:%d fd = %d", mAddr.getIp().data(), mAddr.getPort(), mFd);

    mAcceptIOEvent = IOEvent::createNew(mFd, this);
    mAcceptIOEvent->setReadCallback(readCallback);
    mAcceptIOEvent->enableReadHandling();

    mCloseTriggerEvent = TriggerEvent::createNew(this);
    mCloseTriggerEvent->setTriggerCallback(closeConnectCallback);
}

RtspServer::~RtspServer() {
    if (mListen) {
        mEnv->scheduler()->removeIOEvent(mAcceptIOEvent);
    }

    delete mAcceptIOEvent;
    delete mCloseTriggerEvent;
    sockets::close(mFd);
}

void RtspServer::start() {
    LOGINFO("");
    mListen = true;
    sockets::listen(mFd, 60);
    mEnv->scheduler()->addIOEvent(mAcceptIOEvent);
}

void RtspServer::readCallback(void* arg) {
    auto rtspServer = (RtspServer*)arg;
    rtspServer->handleRead();
}

void RtspServer::handleRead() {
    int clientFd = sockets::accept(mFd);
    if (clientFd < 0) {
        LOGERROR("handle read error, clientFd = %d", clientFd);
        return;
    }

    auto conn = RtspConnection::createNew(this, clientFd);
    conn->setDisConnectionCallback(RtspServer::disconnectCallback, this);
    mConnectMap.insert(std::make_pair(clientFd, conn));
}

void RtspServer::disconnectCallback(void* arg, int clientFd) {
    auto rtspServer = (RtspServer*)arg;
    rtspServer->handleDisconnect(clientFd);
}

void RtspServer::handleDisconnect(int clientFd) {
    std::lock_guard<std::mutex> lock(mMutex);
    mDisconnectList.push_back(clientFd);

    mEnv->scheduler()->addTriggerEvent(mCloseTriggerEvent);
}

void RtspServer::closeConnectCallback(void* arg) {
    auto rtspServer = (RtspServer*)arg;
    rtspServer->handleCloseConnect();
}

void RtspServer::handleCloseConnect() {
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto it = mDisconnectList.begin(); it != mDisconnectList.end(); ++it) {
        int clientFd = *it;

        auto mapIt = mConnectMap.find(clientFd);
        assert(mapIt != mConnectMap.end());
        delete mapIt->second;
        mapIt->second = nullptr;
        mConnectMap.erase(clientFd);
    }

    mDisconnectList.clear();
}
