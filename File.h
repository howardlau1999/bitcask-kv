#pragma once
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <atomic>
#include <string>
#include <mutex>

class File {
public:
    File(const char *aPath, const int aFlags, const long aWriteOffset);
    ~File();
    int append(void* aBuf, size_t aBytes, off_t& outOffsetWritten);
    int read(void* aBuf, size_t aBytes, off_t aOffset) const;

    int truncate() {
        if (m_iFd > 0 && !m_bReadOnly) {
            return ::truncate(m_strFileName.c_str(), 0);
        }
        return -1;
    }

    long writeOffset() {
        return m_lWriteOffset.load();
    }

    std::string fileName() {
        return m_strFileName;
    }

    bool readOnly() {
        return m_bReadOnly.load();
    }

    void disableWriting() {
        m_bReadOnly.store(true);
    }

    void enableWriting() {
        m_bReadOnly.store(false);
    }

private:
    int m_iFd;
    void* m_pMMapped;
    std::atomic<int> m_iMMappedLength;
    std::atomic<bool> m_bReadOnly;
    std::atomic<long> m_lWriteOffset;
    std::string m_strFileName;
    std::mutex m_mMutex;
};