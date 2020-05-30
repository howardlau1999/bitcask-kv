#pragma once
#include <string>
class Cache {
public:
    virtual int get(std::string const& aKey, std::string& outValue) = 0;
    virtual int set(std::string const& aKey, std::string const& aValue) = 0;

    virtual int invalidate(std::string const& aKey) = 0;
    virtual unsigned long cacheHit() = 0;
    virtual unsigned long cacheMiss() = 0;
    virtual ~Cache() = default;
};