#pragma once
#include <mutex>
#include <unordered_map>
#include <list>
#include <memory>
#include <iostream>
#include <atomic>
#include <vector>
#include "Cache.h"

class SimpleKVLRUCache : public Cache {
    struct CacheNode {
        std::string strKey;
        std::string strValue;
        CacheNode(std::string const& aKey, std::string const& aValue) : strKey(aKey), strValue(aValue) {}
    };
    int m_iCapacity;
    std::atomic<unsigned long> m_ulCacheHit;
    std::atomic<unsigned long> m_ulCacheMiss;
    int m_iBuckets;
    std::vector<std::mutex> m_vecMutex;
    std::vector<std::list<CacheNode>> m_vecLstLRU;
    std::vector<std::unordered_map<std::string, std::list<CacheNode>::iterator>> m_vecCache;
public:
    SimpleKVLRUCache(int aiCapacity) : m_iCapacity(aiCapacity), m_ulCacheHit(0), m_ulCacheMiss(0), m_iBuckets(256), m_vecMutex(m_iBuckets) {
        for (int i = 0; i < m_iBuckets; ++i) {
            m_vecLstLRU.emplace_back(std::list<CacheNode>{});
            m_vecCache.emplace_back(std::unordered_map<std::string, std::list<CacheNode>::iterator>{});
        }
    }
    int get(std::string const& aKey, std::string& outValue);
    int set(std::string const& aKey, std::string const& aValue);

    int invalidate(std::string const& aKey);

    unsigned long cacheHit();
    unsigned long cacheMiss();
};