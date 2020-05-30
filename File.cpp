#include "File.h"
#include <cstring>

File::File(const char *aPath, const int aFlags, const long aWriteOffset) : 
    m_iFd(open(aPath, aFlags, 0666)), m_pMMapped(MAP_FAILED), m_iMMappedLength(0), m_lWriteOffset(aWriteOffset), m_strFileName(aPath) {
    if ((aFlags & 0x3) == O_RDWR) {
        m_bReadOnly = false;
    } else if ((aFlags & 0x3) == O_RDONLY) {
        m_bReadOnly = true;
        off_t lFileLength = lseek(m_iFd, 0, SEEK_END);
        m_pMMapped = mmap(NULL, lFileLength, PROT_READ, MAP_PRIVATE, m_iFd, 0);
        m_iMMappedLength = lFileLength;
    }
}

File::~File() {
    if (m_iFd > 0) {
        close(m_iFd);
        m_iFd = -1;
    }
}

int File::read(void* aBuf, size_t aBytes, off_t aOffset) const {
    if (m_bReadOnly && m_pMMapped != MAP_FAILED && aOffset + aBytes < m_iMMappedLength) {
        // Use mmapped address
        memcpy(aBuf, (char*) m_pMMapped + aOffset, aBytes);
        return aBytes;
    }

    // mmaped not available, traditional calls
    size_t sTotalBytesRead = 0;
    while (sTotalBytesRead < aBytes) {
        ssize_t sBytesRead = ::pread(m_iFd, ((char*)aBuf + sTotalBytesRead), aBytes - sTotalBytesRead, aOffset + sTotalBytesRead);
        if (sBytesRead < 0) {
            // Error
            return sBytesRead;
        }

        if (sBytesRead == 0) {
            // EOF
            return sTotalBytesRead;
        }
        sTotalBytesRead += sBytesRead;
    }

    return sTotalBytesRead;
}

int File::append(void* aBuf, size_t aBytes, off_t& outOffset) {
    std::unique_lock<std::mutex> lock(m_mMutex);
    size_t sTotalBytesWritten = 0;
    while (sTotalBytesWritten < aBytes) {
        ssize_t sBytesWritten = ::write(m_iFd, ((char*)aBuf + sTotalBytesWritten), aBytes - sTotalBytesWritten);
        if (sBytesWritten < 0) {
            // Error
            return sBytesWritten;
        }

        sTotalBytesWritten += sBytesWritten;
    }
    
    m_lWriteOffset.fetch_add(sTotalBytesWritten);
    off_t lCurPos = lseek(m_iFd, 0, SEEK_CUR);
    outOffset = lCurPos - sTotalBytesWritten;
    return sTotalBytesWritten;
}