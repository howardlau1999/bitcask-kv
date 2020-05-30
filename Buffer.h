#pragma once
#include <vector>
#include <sys/types.h>
#include <algorithm>
#include <cstring>

class Buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t aInitialSize = kInitialSize)
    : m_vecBuffer(kCheapPrepend + aInitialSize),
        m_iReaderIndex(kCheapPrepend),
        m_iWriterIndex(kCheapPrepend)
    {}

    void swap(Buffer& aRhs) {
        m_vecBuffer.swap(aRhs.m_vecBuffer);
        std::swap(m_iReaderIndex, aRhs.m_iReaderIndex);
        std::swap(m_iWriterIndex, aRhs.m_iWriterIndex);
    }

    size_t readableBytes() const { 
        return m_iWriterIndex - m_iReaderIndex;
    }

    size_t writableBytes() const { 
        return m_vecBuffer.size() - m_iWriterIndex;
    }

    size_t prependableBytes() const { 
        return m_iReaderIndex; 
    }

    const char* peek() const { 
        return begin() + m_iReaderIndex; 
    }

    const char* findEOL() const {
        const void* pEOL = memchr(peek(), '\n', readableBytes());
        return static_cast<const char*>(pEOL);
    }

    const char* findEOL(const char* apStart) const {
        const void* pEOL = memchr(apStart, '\n', beginWrite() - apStart);
        return static_cast<const char*>(pEOL);
    }

    void retrieve(size_t aLen) {
        if (aLen < readableBytes()) {
            m_iReaderIndex += aLen;
        }
        else {
            retrieveAll();
        }
    }

    void retrieveUntil(const char* apEnd) {
        retrieve(apEnd - peek());
    }

    void retrieveAll() {
        m_iReaderIndex = kCheapPrepend;
        m_iWriterIndex = kCheapPrepend;
    }

    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t aLen) {
        std::string result(peek(), aLen);
        retrieve(aLen);
        return result;
    }

    void append(const char* apData, size_t aLen) {
        ensureWritableBytes(aLen);
        std::copy(apData, apData + aLen, beginWrite());
        hasWritten(aLen);
    }

    void append(const void* apData, size_t aLen) {
        append(static_cast<const char*>(apData), aLen);
    }

    void ensureWritableBytes(size_t aLen) {
        if (writableBytes() < aLen) {
            makeSpace(aLen);
        }
    }

    char* beginWrite() { 
        return begin() + m_iWriterIndex; 
    }

    const char* beginWrite() const { 
        return begin() + m_iWriterIndex; 
    }

    void hasWritten(size_t aLen) {
        m_iWriterIndex += aLen;
    }

    void unwrite(size_t aLen) {
        m_iWriterIndex -= aLen;
    }

    void prepend(const void* data, size_t aLen) {
        if (m_iReaderIndex < aLen) {
            abort();
        }
        m_iReaderIndex -= aLen;

        const char* d = static_cast<const char*>(data);
        std::copy(d, d+aLen, begin() + m_iReaderIndex);
    }

    void shrink(size_t aReserveSize) {
        Buffer oOther;
        oOther.ensureWritableBytes(readableBytes() + aReserveSize);
        oOther.append(peek(), readableBytes());
        swap(oOther);
    }

    size_t internalCapacity() const {
        return m_vecBuffer.capacity();
    }
    ssize_t readFd(int fd, int* savedErrno);

private:

    char* begin() { 
        return &*m_vecBuffer.begin();
    }

    const char* begin() const {
        return &*m_vecBuffer.begin();
    }

    void makeSpace(size_t aLen) {
        if (writableBytes() + prependableBytes() < aLen + kCheapPrepend) {
            m_vecBuffer.resize(m_iWriterIndex + aLen);
        } else {
            size_t ulReadable = readableBytes();
            std::copy(begin() + m_iReaderIndex,
                    begin() + m_iWriterIndex,
                    begin() + kCheapPrepend);
            m_iReaderIndex = kCheapPrepend;
            m_iWriterIndex = m_iReaderIndex + ulReadable;
        }
    }

    std::vector<char> m_vecBuffer;
    size_t m_iReaderIndex;
    size_t m_iWriterIndex;
};