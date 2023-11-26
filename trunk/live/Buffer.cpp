#include "Buffer.h"

#include <algorithm>
#include <cstdlib>

#include <sys/socket.h>

#include "../scheduler/SocketsOptions.h"

const int Buffer::initialSize = 1024;
const char* Buffer::kCRLF = "\r\n";

Buffer::Buffer()
    : mBufferSize(initialSize)
    , mReadIndex(0)
    , mWriteIndex(0) {
    mBuffer = (char*)malloc(mBufferSize);
}

Buffer::~Buffer() {
    free(mBuffer);
}

int Buffer::read(int fd) {
    char extraBuf[65536];
    const int writable = writableBytes();
    const int n = ::recv(fd, extraBuf, sizeof(extraBuf), 0);
    if (n <= 0) {
        return -1;
    } else if (n <= writable) {
        std::copy(extraBuf, extraBuf + n, beginWrite());
        mWriteIndex += n;
    } else {
        std::copy(extraBuf, extraBuf + writable, beginWrite());
        mWriteIndex += writable;
        append(extraBuf + writable, n - writable);
    }

    return n;
}

int Buffer::write(int fd) {
    return sockets::write(fd, peek(), readableBytes());
}
