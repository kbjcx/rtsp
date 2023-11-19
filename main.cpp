#include "rtp.h" 
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstdio>

#define AAC_FILE_NAME   "./test.aac"
#define H264_FILE_NAME  "./test.h264"
#define SERVER_PORT     8554
#define BUFFER_MAX_SIZE (1024 * 1024)

static int createTcpSocket() {
    int sockfd = -1;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, static_cast<const void*>(&on), sizeof(on));

    return sockfd;
}

static int bindSocketAddr(int sockfd, const char* ip, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(sockaddr)) < 0) {
        return -1;
    }

    return 0;
}

static int acceptClient(int sockfd, char* ip, uint16_t* port) {
    int clientfd = -1;
    socklen_t len = 0;
    sockaddr_in addr{};

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
    if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) {
        return 1;
    }

    return 0;
}

static inline int startCode4(char* buffer) {
    if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1) {
        return 1;
    }

    return 0;
}

static char* findNextStartCode(char* buffer, int len) {
    if (len < 3) {
        return nullptr;
    }
    
    for (int i = 0; i < len - 3; ++i) {
        if (startCode3(buffer) || startCode4(buffer)) {
            return buffer;
        }
        ++buffer;
    }

    if (startCode3(buffer)) {
        return buffer;
    }

    return nullptr;
}

static int getFrameFromH264(FILE* fp, char* frame, int size) {
    int rSize = 0, frameSize = 0;
    char* nextStartCode = nullptr;

    if (fp == nullptr) {
        return -1;
    }

    rSize = fread(frame, 1, size, fp);
    if (startCode3(frame) == 0 && startCode4(frame) == 0) {
        return -1;
    }

    nextStartCode = findNextStartCode(frame + 3, size - 3);
    if (nextStartCode == nullptr) {
        return -1;
    } else {
        frameSize = nextStartCode - frame;
        fseek(fp, frameSize - rSize, SEEK_CUR);
    }

    return frameSize;
}

struct AdtsHeader {
    unsigned int syncword;
    unsigned int id;
    unsigned int layer;
    unsigned int protectionAbsent;
    unsigned int profile;
    unsigned int samplingFreqIndex;
    unsigned int privateBit;
    unsigned int channelCfg;
    unsigned int originCopy;
    unsigned int home;

    unsigned int copyrightIdentificationBit;
    unsigned int copyrightIdentificationStart;
    unsigned int aacFrameLength;
    unsigned int adtsBufferFullness;

    unsigned int numberOfRawDataBlockInFrame;
}; // struct AdtsHeader

static int parseAdtsHeader(uint8_t* in, AdtsHeader* res) {
    static int frameNumber = 0;

    memset(res, 0, sizeof(AdtsHeader));

    if (in[0] == 0xFF && (in[1] & 0xF0) == 0xF0) {
        res->id = ((unsigned int)in[1] & 0x08) >> 3;
        res->layer = ((unsigned int)in[1] & 0x06) >> 1;
        res->protectionAbsent = ((unsigned int)in[1] & 0x01);
        res->profile = ((unsigned int)in[2] & 0xC0) >> 6;
        res->samplingFreqIndex = ((unsigned int)in[2] & 0x3C) >> 2;
        res->privateBit = ((unsigned int)in[2] & 0x02) >> 1;
        res->channelCfg = (((unsigned int)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xC0) >> 6);
        res->originCopy = ((unsigned int)in[3] & 0x20) >> 5;
        res->home = ((unsigned int)in[3] & 0x01) >> 4;
        res->copyrightIdentificationBit = ((unsigned int)in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = ((unsigned int)in[3] & 0x04) >> 2;
        res->aacFrameLength = (((unsigned int)in[3] & 0x03) << 11) | 
        (((unsigned int)in[4] & 0xFF) << 3) |
        (((unsigned int)in[5] & 0xE0) >> 5);
        res->adtsBufferFullness = (((unsigned int)in[5] & 0x1F) << 6) | (((unsigned int)in[6] & 0xFC) >> 2);
        res->numberOfRawDataBlockInFrame = ((unsigned int)in[6] & 0x03);

    } else {
        printf("failed to parse adts header\n");
        return -1;
    }
}