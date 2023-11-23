#ifndef RTSPSERVER_SOCKETSOPTIONS_H
#define RTSPSERVER_SOCKETSOPTIONS_H

#include <cstdint>
#include <string>

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <WS2tcpip.h>
#include <WinSock2.h>
#endif

namespace sockets {
int createTcpSocket(); // 默认创建非阻塞socket
int createUdpSocket();
bool bind(int sockfd, std::string ip, uint16_t port);
bool listen(int sockfd, int backlog);
int accept(int sockfd);

int write(int sockfd, const void* buf, int size);
int sendto(int sockfd, const void* buf, int len, const sockaddr* destAddr);
int setNonBlock(int sockfd);
int setBlock(int sockfd, int writeTimeout);
void setReuseAddr(int sockfd, int on);
void setReusePort(int sockfd);
void setNonBlockAndCloseOnExec(int sockfd);
void ignoreSigPipeOnSocket(int sockfd);
void setNoDelay(int sockfd);
void setKeepAlive(int sockfd);
void setNoSigPipe(int sockfd);
void setSendBufferSize(int sockfd, int size);
void setRecvBufferSize(int sockfd, int size);
std::string getPeerIp(int sockfd);
uint16_t getPeerPort(int sockfd);
int getPeerAddr(int sockfd, sockaddr_in* addr);
void close(int sockfd);
bool connect(int sockfd, std::string ip, uint16_t port, int timeout);
std::string getLocalIp();
}; // namespace sockets

#endif
