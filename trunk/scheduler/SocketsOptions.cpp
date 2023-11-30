#include "SocketsOptions.h"

#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>

#include <cstddef>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifndef WIN32
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#include "../base/log.h"

namespace sockets {
int createTcpSocket() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

    return sockfd;
} // 默认创建非阻塞socket

int createUdpSocket() {
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    return sockfd;
}

bool bind(int sockfd, std::string ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (::bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOGERROR("::bind error, fd = %d, ip = %s, port = %d\n", sockfd, ip.c_str(), port);
        return false;
    }
    return true;
}

bool listen(int sockfd, int backlog) {
    if (::listen(sockfd, backlog) < 0) {
        LOGERROR("::listen error, fd = %d, backlog = %d\n", sockfd, backlog);
        return false;
    }
    return true;
}

int accept(int sockfd) {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(sockaddr_in);

    int clientFd = ::accept(sockfd, (sockaddr*)&addr, &addrlen);
    setNonBlockAndCloseOnExec(clientFd);
    ignoreSigPipeOnSocket(clientFd);

    return clientFd;
}

int write(int sockfd, const void* buf, int size) {
    return ::write(sockfd, buf, size);
}

int sendto(int sockfd, const void* buf, int len, const sockaddr* destAddr) {
    socklen_t addrlen = sizeof(sockaddr);
    return ::sendto(sockfd, (char*)buf, len, 0, destAddr, addrlen);
}

int setNonBlock(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}

/**
 * @brief 设置文件描述符阻塞与发送超时时间
 *
 * @param sockfd
 * @param writeTimeout(ms)
 * @return int
 */
int setBlock(int sockfd, int writeTimeout) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    if (writeTimeout > 0) {
        timeval timeout = {writeTimeout / 1000, (writeTimeout % 1000) * 1000};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    }
    return 0;
}
void setReuseAddr(int sockfd, int on) {
    int optVal = on ? 1 : 0;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optVal, sizeof(optVal));
}

void setReusePort(int sockfd) {
#ifdef SO_REUSEPORT
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const void*)&on, sizeof(on));
#endif
}

void setNonBlockAndCloseOnExec(int sockfd) {
    // non-block
    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret &= fcntl(sockfd, F_SETFD, flags);
}

void ignoreSigPipeOnSocket(int sockfd) {
    int optVal = 1;
    setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, (const char*)&optVal, sizeof(optVal));
}

void setNoDelay(int sockfd) {
#ifdef TCP_NODELAY
    int on = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const chat*)&on, sizeof(on));
#endif
}

void setKeepAlive(int sockfd) {
    int on = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&on, sizeof(on));
}

void setNoSigPipe(int sockfd) {
#ifdef SO_NOSIGPIPE
    int on = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (const char*)&on, sizeof(on));
#endif
}

void setSendBufferSize(int sockfd, int size) {
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&size, sizeof(size));
}

void setRecvBufferSize(int sockfd, int size) {
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&size, sizeof(size));
}

std::string getPeerIp(int sockfd) {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(sockaddr_in);
    if (getpeername(sockfd, (sockaddr*)&addr, &addrlen) == 0) {
        return inet_ntoa(addr.sin_addr);
    }

    return "0.0.0.0";
}

uint16_t getPeerPort(int sockfd) {
    sockaddr_in addr{};
    socklen_t addrlen = sizeof(sockaddr_in);
    if (getpeername(sockfd, (sockaddr*)&addr, &addrlen) == 0) {
        return ntohs(addr.sin_port);
    }

    return 0;
}

int getPeerAddr(int sockfd, sockaddr_in* addr) {
    socklen_t addrlen = sizeof(sockaddr_in);
    return getpeername(sockfd, (sockaddr*)addr, &addrlen);
}

void close(int sockfd) {
    close(sockfd);
}

bool connect(int sockfd, std::string ip, uint16_t port, int timeout) {
    bool isConnected = true;
    if (timeout > 0) {
        setNonBlock(sockfd);
    }

    sockaddr_in addr{};
    socklen_t addrlen = sizeof(sockaddr_in);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(sockfd, (sockaddr*)&addr, addrlen) < 0) {
        if (timeout > 0) {
            isConnected = false;
            fd_set fdWrite;
            FD_ZERO(&fdWrite);
            FD_SET(sockfd, &fdWrite);
            timeval tv = {timeout / 1000, (timeout % 1000) * 1000};
            select(sockfd + 1, nullptr, &fdWrite, nullptr, &tv);
            if (FD_ISSET(sockfd, &fdWrite)) {
                isConnected = true;
            }
            setBlock(sockfd, 0);
        } else {
            isConnected = false;
        }
    }

    return isConnected;
}

std::string getLocalIp() {
    return "0.0.0.0";
}
}; // namespace sockets
