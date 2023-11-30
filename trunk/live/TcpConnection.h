#ifndef RTSPSERVER_TCPCONNECTION_H
#define RTSPSERVER_TCPCONNECTION_H

#include "../scheduler/Event.h"
#include "../scheduler/UsageEnvironment.h"
#include "Buffer.h"

class TcpConnection {
public:
    using DisConnectionCallback = void (*)(void*, int);

    TcpConnection(UsageEnvironment* env, int clientFd);
    virtual ~TcpConnection();

    void setDisConnectionCallback(DisConnectionCallback cb, void* arg);

protected:
    void enableReadHandling();
    void enableWriteHandling();
    void enableErrorHandling();

    void disableReadHandling();
    void disableWriteHandling();
    void disableErrorHandling();

    void handleRead();
    virtual void handleReadBytes();
    virtual void handleWrite();
    virtual void handleError();

    void handleDisConnect();

private:
    static void readCallback(void* arg);
    static void writeCallback(void* arg);
    static void errorCallback(void* arg);

protected:
    UsageEnvironment* mEnv;
    int mClientFd;
    IOEvent* mClientIOEvent;
    DisConnectionCallback mDisConnectionCallback;
    void* mArg;
    Buffer mInputBuffer;
    Buffer mOutputBuffer;
    char mBuffer[2048];
};

#endif
