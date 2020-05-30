#pragma once
#include <string>
#include <sys/stat.h>
#include <unistd.h>
namespace utils {
    ssize_t getFileSize(std::string const& aPath);

    bool fileExists(std::string const& aPath);
    bool haveReadAccess(std::string const& aPath);
    bool haveWriteAccess(std::string const& aPath);

    bool haveWRAccess(std::string const& aPath);

    int getBucketIndex(int aBucketNum, std::string const& aKey);
};