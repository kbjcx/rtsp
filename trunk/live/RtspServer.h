#ifndef RTSPSERVER_RTSPSERVER_H
#define RTSPSERVER_RTSPSERVER_H

#include <map>
#include <mutex>
#include "../scheduler/UsageEnvironment.h"
#include "../scheduler/Event.h"
#include "./MediaSession.h"
#include "./InetAddress.h"

class MediaSessionManager;
class RtspConnection;
class RtspServer {
public:
    static RtspServer* createNew(UsageEnvironment* env, MediaSessionManager* sessionManager,
                                 Ipv4Address& addr);
    RtspServer(UsageEnvironment* env, MediaSessionManager* sessionManager, Ipv4Address& addr);
    ~RtspServer();

public:
    void start();
    UsageEnvironment* env() const;

private:
    static void readCallback(void*);
    void handleRead();

    static void disconnectCallback(void* arg, int clientFd);
    void handleDisconnect(int clientFd);

    static void closeConnectCallback(void* arg);
    void handleCloseConnect();

private:
    MediaSessionManager* mSessionManager;
    UsageEnvironment* mEnv;
    int mFd;
    Ipv4Address mAddr;
    bool mListen;
    IOEvent* mAcceptIOEvent;
    std::mutex mMutex;

    std::map<int, RtspConnection*> mConnectMap; // 维护所有被创建的连接
    std::vector<int> mDisconnectList; // 所有被取消的连接
    TriggerEvent* mCloseTriggerEvent; // 关闭连接的触发事件
};

#endif
