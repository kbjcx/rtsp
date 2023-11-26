#ifndef RTSPSERVER_RTP_H
#define RTSPSERVER_RTP_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define RTP_VERSION           2
#define RTP_PAYLOAD_TYPE_H264 96
#define RTP_PAYLOAD_TYPE_AAC  97
#define RTP_HEADER_SIZE       12
#define RTP_MAX_PACKET_SIZE   1400

/**
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|0 1 2 3 4 5 6 7|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|CC=4   |M|PT=7         |sequence number=16             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           time stamp                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            synchronixation source identifier(SSRC)            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             contributing source identifiers(CSRC)             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       [CSRC(0 ~ 15个)]						|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   payload(video, audio ...)                   |
+                                           +-+-+-+-+-+-+-+-+-+-+
|                                           |       padding     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**/
struct RtpHeader {
    // byte
    uint8_t csrcLen   : 4;
    uint8_t extension : 1;
    uint8_t padding   : 1;
    uint8_t version   : 2;

    // byte2
    uint8_t 
};

#endif // !RTSPSERVER_RTP_H
