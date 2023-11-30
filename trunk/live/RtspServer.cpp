#include "InetAddress.h"
#include "RtspServer.h"

RtspServer* RtspServer::createNew(
    UsageEnvironment* env, MediaSessionManager* sessionManager, Ipv4Address& addr) {
    return new RtspServer(env, sessionManager, addr);
}

RtspServer::RtspServer(
    UsageEnvironment* env, MediaSessionManager* sessionManager, Ipv4Address& addr)
    : mEnv(env)
    , mSessionManager(sessionManager)
    , mAddr(addr) {}
