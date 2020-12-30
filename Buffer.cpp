#include "Buffer.h"
#include <sys/uio.h>
const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int aFd, int* apSavedErrno) {
    char pExtraBuf[65536];
    struct iovec stIOVec[2];
    const size_t ulWritable = writableBytes();
    stIOVec[0].iov_base = begin() + m_iWriterIndex;
    stIOVec[0].iov_len = ulWritable;
    stIOVec[1].iov_base = pExtraBuf;
    stIOVec[1].iov_len = sizeof(pExtraBuf);

    const int iIOVecCnt = (ulWritable < sizeof(pExtraBuf)) ? 2 : 1;
    const ssize_t iBytesRead = ::readv(aFd, stIOVec, iIOVecCnt);

    if (iBytesRead < 0) {
        if (apSavedErrno != NULL) {
            *apSavedErrno = errno;
        }
        perror("readFd");
        abort();
    } else if (iBytesRead <= (long int)ulWritable) {
        m_iWriterIndex += iBytesRead;
    } else {
        m_iWriterIndex = m_vecBuffer.size();
        append(pExtraBuf, iBytesRead - ulWritable);
    }

    return iBytesRead;
}