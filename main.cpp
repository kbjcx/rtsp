#include "rtp.h" 
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstdio>
#include <time.h>

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

        return 0;
    } else {
        printf("failed to parse adts header\n");
        return -1;
    }
}

static int rtpSendAACFrame(int clientSockfd, RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize) {
    int ret = 0;

    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5;
    rtpPacket->payload[3] = (frameSize & 0x1F) << 3;

    memcpy(rtpPacket->payload + 4, frame, frameSize);

    ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, frameSize + 4, 0x02);

    if (ret < 0) {
        printf("failed to send rtp packet \n");
        return -1;
    }

    rtpPacket->rtpHeader.seq++;

    rtpPacket->rtpHeader.timestamp += 1025;

    return 0;
}

static int rtpSendH264Frame(int clientSockfd, RtpPacket* rtpPacket, char* frame, uint32_t frameSize) {
    uint8_t naluType = 0;
    int sendByte = 0;
    int ret = -1;

    naluType = frame[0];

    printf("%s frameSize = %d \n", __FUNCTION__, frameSize);
    if (frameSize <= RTP_MAX_PACKET_SIZE) {
        // 整包发送
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, frameSize, 0x00);
        if (ret < 0) {
            return -1;
        }

        rtpPacket->rtpHeader.seq++;

        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) {
            // TODO
        }
    } else {
        int pktNum = frameSize / RTP_MAX_PACKET_SIZE;
        int remainPktSize = frameSize % RTP_MAX_PACKET_SIZE;
        int pos = 1;

        for (int i = 0; i < pktNum; ++i) {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;

            if (i == 0) {
                rtpPacket->payload[1] |= 0x80;
            } else if (remainPktSize == 0 && i == pktNum - 1) {
                rtpPacket->payload[1] |= 0x40;
            }

            memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PACKET_SIZE);
            ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, RTP_MAX_PACKET_SIZE + 2, 0x00);
            if (ret < 0) {
                return -1;
            }

            rtpPacket->rtpHeader.seq++;
            sendByte += ret;
            pos += RTP_MAX_PACKET_SIZE;
        }

        if (remainPktSize > 0) {
            rtpPacket->payload[0] = naluType & 0x60 | 28;
            rtpPacket->payload[1] = (naluType & 0x1F) | 0x40;

            memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize);
            ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, remainPktSize + 2, 0x00);
            if (ret < 0) {
                return -1;
            }

            rtpPacket->rtpHeader.seq++;
            sendByte += ret;
        }
    }

    return sendByte;
}

static int handleCmdOptions(char* res, int cseq) {
    sprintf(res, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
            "\r\n",
            cseq);
    
    return 0;
}

static int handleCmdDescribe(char* res, int cseq, char* url) {
    char sdp[500];
    char localIp[100];

    sscanf(url, "rtsp://%[^:]:", localIp);
    sprintf(sdp, "v=0\r\n"
            "o=- 9%ld 1 IN IP4 %s\r\n"
            "t=0 0"
            "a=control:*\r\n"
            "m=video 0 RTP/AVP/TCP 96\r\n"
            "a=rtpmap:96 H264/90000\r\n"
            "a=control:track0\r\n"
            "m=audio 1 RTP/AVP/TCP 97\r\n"
            "a=rtpmap:97 mpeg4-generic/44100/2\r\n"
            "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n"
            "a=control:track1\r\n", 
            time(nullptr), localIp);
    
    sprintf(res, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Content-Base: %s\r\n"
            "Content-type: application/sdp\r\n"
            "Content-length: %zu\r\n"
            "\r\n"
            "%s", 
            cseq, url, strlen(sdp), sdp);
    
    return 0;
}

static int handleCmdSetup(char* res, int cseq) {
    if (cseq == 3) {
        sprintf(res, "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
                "Session: 66334873\r\n"
                "\r\n", 
                cseq);
    } else if (cseq == 4) {
        sprintf(res, "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n"
                "Session: 66334873\r\n"
                "\r\n", 
                cseq);
    }

    return 0;
}

static int handleCmdPlay(char* res, int cseq) {
    sprintf(res, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Range: npt=0.000-\r\n"
            "Session: 66334873; timeout=10\r\n"
            "\r\n", 
            cseq);
    
    return 0;
}

static int doClient(int clientSockfd, const char* clientIp, int clientPort) {
    char method[40];
    char url[100];
    char version[40];
    int CSeq = 0;

    char* rBuf = (char*)malloc(BUFFER_MAX_SIZE);
    char* sBuf = (char*)malloc(BUFFER_MAX_SIZE);

    while (true) {
        int recvLen;
        recvLen = recv(clientSockfd, rBuf, BUFFER_MAX_SIZE, 0);
        if (recvLen <= 0) {
            break;
        }

        rBuf[recvLen] = '\0';
        printf("rbuf = %s \n", rBuf);

        const char* sep = "\n";

        char* line = strtok(rBuf, sep);
        while (line) {
            if (strstr(line, "OPTIONS") || strstr(line, "DESCRIBE") || strstr(line, "SETUP") || strstr(line, "PLAY")) {
                if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3) {
                    // TODO error
                }
            } else if (strstr(line, "CSeq")) {
                if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1) {
                    // TODO error
                }
            } else if (!strncmp(line, "Transport:", strlen("Transport:"))) {
                if (sscanf(line, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n") != 0) {
                    // TODO error
                    printf("parse Transport error\n");
                }
            }

            line = strtok(nullptr, sep);
        }

        if (!strcmp(method, "OPTIONS")) {
            if (handleCmdOptions(sBuf, CSeq)) {
                break;
            }
        } else if (!strcmp(method, "DESCRIBE")) {
            if (handleCmdDescribe(sBuf, CSeq, url)) {
                break;
            }
        } else if (!strcmp(method, "SETUP")) {
            if (handleCmdSetup(sBuf, CSeq)) {
                break;
            }
        } else if (!strcmp(method, "PLAY")) {
            if (handleCmdPlay(sBuf, CSeq)) {
                break;
            }
        } else {
            printf("Unknown method: %s\n", method);
            break;
        }

        printf("sBuf = %s\n", sBuf);

        send(clientSockfd, sBuf, strlen(sBuf), 0);

        if (!strcmp(method, "PLAY")) {
            
        }
    }
}