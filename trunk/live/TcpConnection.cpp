#include "TcpConnection.h"

#include "../base/log.h"
#include "../scheduler/SocketsOptions.h"

TcpConnection::TcpConnection(UsageEnvironment* env, int clientFd)
    : mEnv(env)
    , mClientFd(clientFd) {
    mClientIOEvent = IOEvent::createNew(clientFd, this);
    mClientIOEvent->setReadCallback(readCallback);
    mClientIOEvent->setWriteCallback(writeCallback);
    mClientIOEvent->setErrorCallback(errorCallback);
    mClientIOEvent->enableReadHandling();

    mEnv->scheduler()->addIOEvent(mClientIOEvent);
}

TcpConnection::~TcpConnection() {
    mEnv->scheduler()->removeIOEvent(mClientIOEvent);
    delete mClientIOEvent;
    sockets::close(mClientFd);
}

void TcpConnection::setDisConnectionCallback(DisConnectionCallback cb, void* arg) {
    mDisConnectionCallback = cb;
    mArg = arg;
}

void TcpConnection::enableReadHandling() {
    if (mClientIOEvent->isReadHandling()) {
        return;
    }

    mClientIOEvent->enableReadHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::enableWriteHandling() {
    if (mClientIOEvent->isWriteHandling()) {
        return;
    }

    mClientIOEvent->enableWriteHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::enableErrorHandling() {
    if (mClientIOEvent->isErrorHandling()) {
        return;
    }

    mClientIOEvent->enableErrorHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::disableReadHandling() {
    if (!mClientIOEvent->isReadHandling()) {
        return;
    }

    mClientIOEvent->disableReadHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::disableWriteHandling() {
    if (!mClientIOEvent->isWriteHandling()) {
        return;
    }

    mClientIOEvent->disableWriteHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::disableErrorHandling() {
    if (!mClientIOEvent->isErrorHandling()) {
        return;
    }

    mClientIOEvent->disableErrorHandling();
    mEnv->scheduler()->updateIOEvent(mClientIOEvent);
}

void TcpConnection::handleRead() {
    int ret = mInputBuffer.read(mClientFd);
    if (ret <= 0) {
        LOGERROR("read error, fd = %d, ret = %d", mClientFd, ret);
        handleDisConnect();
        return;
    }

    handleReadBytes();
}

void TcpConnection::handleReadBytes() {
    LOGINFO("%s handle read bytes", __FUNCTION__);
    mInputBuffer.retrieveAll();
}

void TcpConnection::handleDisConnect() {
    if (mDisConnectionCallback) {
        mDisConnectionCallback(mArg, mClientFd);
    }
}

void TcpConnection::handleWrite() {
    LOGINFO("%s handle write", __FUNCTION__);
    mOutputBuffer.retrieveAll();
}

void TcpConnection::handleError() {
    LOGINFO("%s handle error", __FUNCTION__);
}

void TcpConnection::readCallback(void* arg) {
    auto tcpConnection = (TcpConnection*)arg;
    tcpConnection->handleRead();
}

void TcpConnection::writeCallback(void* arg) {
    auto tcpConnection = (TcpConnection*)arg;
    tcpConnection->handleWrite();
}

void TcpConnection::errorCallback(void* arg) {
    auto tcpConnection = (TcpConnection*)arg;
    tcpConnection->handleError();
}
