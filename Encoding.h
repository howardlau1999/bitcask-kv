#pragma once
#include <vector>
#include <string>
#include <cstring>
#include <sys/types.h>
class Encoding {
public:
    static std::vector<char> encodeIndex(std::string aKey, size_t aValueLen, off_t aValueOffset, unsigned long aTimeStamp) {
        std::vector<char> vecEncoded;
        size_t ulKeySize = aKey.size();
        vecEncoded.resize(sizeof(aValueLen) * 2 + sizeof(aValueOffset) + sizeof(aTimeStamp) + aKey.size());
        auto ptr = &vecEncoded[0];
        memcpy(ptr, &aTimeStamp, sizeof(aTimeStamp));
        ptr += sizeof(aTimeStamp);
        memcpy(ptr, &ulKeySize, sizeof(ulKeySize));
        ptr += sizeof(ulKeySize);
        memcpy(ptr, &aKey[0], aKey.size());
        ptr += aKey.size();
        memcpy(ptr, &aValueLen, sizeof(aValueLen));
        ptr += sizeof(aValueLen);
        memcpy(ptr, &aValueOffset, sizeof(aValueOffset));
        ptr += sizeof(aValueOffset);
        return vecEncoded;
    }

    static std::vector<char> encodeRecord(std::string aKey, std::string aValue, unsigned long aTimeStamp) {
        std::vector<char> vecEncoded;
        size_t ulKeySize = aKey.size();
        size_t ulValueLen = aValue.size();
        vecEncoded.resize(sizeof(size_t) * 2 + sizeof(aTimeStamp) + aKey.size() + aValue.size());
        auto ptr = &vecEncoded[0];
        memcpy(ptr, &aTimeStamp, sizeof(aTimeStamp));
        ptr += sizeof(aTimeStamp);
        memcpy(ptr, &ulKeySize, sizeof(ulKeySize));
        ptr += sizeof(ulKeySize);
        memcpy(ptr, &aKey[0], aKey.size());
        ptr += aKey.size();
        memcpy(ptr, &ulValueLen, sizeof(ulValueLen));
        ptr += sizeof(ulValueLen);
        memcpy(ptr, &aValue[0], aValue.size());
        ptr += sizeof(aValue.size());
        return vecEncoded;
    }
};