#ifndef RTSPSERVER_INETADDRESS_H
#define RTSPSERVER_INETADDRESS_H

#include <stdint.h>
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

class Ipv4Address {
public:
    Ipv4Address();
    Ipv4Address(std::string ip, uint16_t port);
    void setAddr(std::string ip, uint16_t port);
    std::string getIp();
    uint16_t getPort();
    sockaddr* getAddr();

private:
    std::string mIp;
    uint16_t mPort;
    sockaddr_in mAddr;
};

#endif
