#include "rtp.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

#define AAC_FILE_NAME   "./test.aac"
#define H264_FILE_NAME  "./test.h264"
#define SERVER_PORT     8554
#define BUFFER_MAX_SIZE (1024 * 1024)

static int createTcpSocket() {
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

static int bindSocketAddr(int sockfd, const char* ip, int port) {
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr)) < 0) {
        return -1;
    }

    return 0;
}

static int acceptClient(int sockfd, char* ip, int16_t* port) {
    int clientfd;
    socklen_t len = 0;
    sockaddr_in addr;

    memset(&addr, 0, sizeof(sockaddr_in));
    len = sizeof(sockaddr_in);

    clientfd = accept(sockfd, (sockaddr*)&addr, &len);
    if (clientfd < 0) {
        return -1;
    }

    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);

    return clientfd;
}

static inline int startCode3(char* buffer) {
    
}
