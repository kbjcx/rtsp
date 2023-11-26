#ifndef RTSPSERVER_BUFFER_H
#define RTSPSERVER_BUFFER_H

#include <algorithm>
#include <cassert>

#include <stdint.h>
#include <stdlib.h>

class Buffer {
public:
    static const int initialSize;
    explicit Buffer();
    ~Buffer();

    /**
     * @brief 还剩下多少数据未读
     *
     * @return int
     */
    int readableBytes() const {
        return mWriteIndex - mReadIndex;
    }

    /**
     * @brief 还能接受socket多少数据
     *
     * @return int
     */
    int writableBytes() const {
        return mBufferSize - mWriteIndex;
    }

    /**
     * @brief 已经读完的数据，可回收利用
     *
     * @return int
     */
    int prependableBytes() const {
        return mReadIndex;
    }

    /**
     * @brief 当前正在处理的数据
     *
     * @return char*
     */
    char* peek() {
        return begin() + mReadIndex;
    }

    const char* peek() const {
        return begin() + mReadIndex;
    }

    const char* findCRLF() const {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findCRLF(const char* start) const {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findLastCRLF() const {
        const char* crlf = std::find_end(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    /**
     * @brief 重新从头读取数据
     *
     */
    void retrieveReadZero() {
        mReadIndex = 0;
    }

    /**
     * @brief 该长度的数据已被处理，跳过该部分
     *
     * @param len
     */
    void retrieve(int len) {
        assert(readableBytes() >= len);
        if (len < readableBytes()) {
            mReadIndex += len;
        } else {
            retrieveAll();
        }
    }

    /**
     * @brief 当前的处理进度到达了end处
     *
     * @param end
     */
    void retrieveUntil(const char* end) {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    /**
     * @brief 数据全部处理完毕，清空索引
     *
     */
    void retrieveAll() {
        mReadIndex = 0;
        mWriteIndex = 0;
    }

    char* beginWrite() {
        return begin() + mWriteIndex;
    }

    const char* beginWrite() const {
        return begin() + mWriteIndex;
    }

    /**
     * @brief 删除已接收且还未处理的len长度的数据
     *
     * @param len
     */
    void unwrite(int len) {
        assert(len <= readableBytes());
        mWriteIndex -= len;
    }

    /**
     * @brief 确保有len长度的空间可以接收数据
     *
     * @param len
     */
    void ensureWritableBytes(int len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    /**
     * @brief 扩充接收数据的空间
     *
     * @param len
     */
    void makeSpace(int len) {
        if (writableBytes() + prependableBytes() < len) {
            mBufferSize = mWriteIndex + len;
            mBuffer = (char*)realloc(mBuffer, mBufferSize);
        } else {
            int readable = readableBytes();
            std::copy(begin() + mReadIndex, begin() + mWriteIndex, begin());
            mReadIndex = 0;
            mWriteIndex = mReadIndex + readable;
            assert(mWriteIndex == readableBytes());
        }
    }

    /**
     * @brief 在已有数据后面添加数据，会自动扩容
     *
     * @param data
     * @param len
     */
    void append(const char* data, int len) {
        ensureWritableBytes(len);
        assert(len <= writableBytes());
        std::copy(data, data + len, beginWrite());
        mWriteIndex += len;
    }

    void append(const void* data, int len) {
        append((const char*)data, len);
    }

    int read(int fd);

    int write(int fd);

private:
    char* begin() {
        return mBuffer;
    }

    const char* begin() const {
        return mBuffer;
    }

private:
    char* mBuffer;
    int mBufferSize;
    int mReadIndex;
    int mWriteIndex;

    static const char* kCRLF;
};

#endif
