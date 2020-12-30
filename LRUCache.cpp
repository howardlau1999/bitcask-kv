#include "LRUCache.h"
#include "Utils.h"
int SimpleKVLRUCache::get(std::string const& aKey, std::string& outValue) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mCache = m_vecCache[iBucketIdx];
    auto& lstLRU = m_vecLstLRU[iBucketIdx];
    auto it = mCache.find(aKey);
    if (it == mCache.end()) {
        ++m_ulCacheMiss;
        return -1;
    }
    ++m_ulCacheHit;
    lstLRU.splice(lstLRU.begin(), lstLRU, it->second);
    mCache[it->first] = lstLRU.begin();
    outValue = lstLRU.begin()->strValue;
    return 0;
}

int SimpleKVLRUCache::set(std::string const& aKey, std::string const& aValue) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mCache = m_vecCache[iBucketIdx];
    auto& lstLRU = m_vecLstLRU[iBucketIdx];

    auto it = mCache.find(aKey);
    if (it == mCache.end()) {
        if (lstLRU.size() >= (std::size_t)m_iCapacity) {
            mCache.erase(lstLRU.back().strKey);
            lstLRU.pop_back();
        }
        lstLRU.emplace_front(CacheNode(aKey, aValue));
        mCache[aKey] = lstLRU.begin();
    } else {
        lstLRU.splice(lstLRU.begin(), lstLRU, it->second);
        mCache[it->first] = lstLRU.begin();
        lstLRU.begin()->strValue = aValue;
    }
    return 0;
}

int SimpleKVLRUCache::invalidate(std::string const& aKey) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mCache = m_vecCache[iBucketIdx];
    auto& lstLRU = m_vecLstLRU[iBucketIdx];

    auto it = mCache.find(aKey);
    if (it != mCache.end()) {
        lstLRU.erase(it->second);
        mCache.erase(it);
    }
    return 0;
}

unsigned long SimpleKVLRUCache::cacheHit() {
    return m_ulCacheHit.load();
}

unsigned long SimpleKVLRUCache::cacheMiss() {
    return m_ulCacheMiss.load();
}