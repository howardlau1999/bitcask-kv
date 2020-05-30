#include "Utils.h"
namespace utils {
    ssize_t getFileSize(std::string const& aPath) {
        struct stat stStat;
        if (stat(aPath.c_str(), &stStat)) {
            return -1;
        }

        return stStat.st_size;
    }

    bool fileExists(std::string const& aPath) {
        return access(aPath.c_str(), F_OK) == 0;
    }

    bool haveReadAccess(std::string const& aPath) {
        return access(aPath.c_str(), R_OK) == 0;
    }

    bool haveWriteAccess(std::string const& aPath) {
        return access(aPath.c_str(), W_OK) == 0;
    }

    bool haveWRAccess(std::string const& aPath) {
        return access(aPath.c_str(), R_OK | W_OK) == 0;
    }

    int getBucketIndex(int aBucketNum, std::string const& aKey) {
        static std::hash<std::string> hasher;
        return hasher(aKey) % aBucketNum;
    }
};