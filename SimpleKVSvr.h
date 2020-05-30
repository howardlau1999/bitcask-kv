#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <pthread.h>
#include "Socket.h"
#include "TcpConnection.h"
#include <algorithm>
#include <atomic>
#include <iostream>
#include "Cache.h"
#include "File.h"
#include "EventLoopThreadPool.h"
#include <unordered_map>
struct SimpleKVRecord {
    int iFileId;
    size_t lValueOffset;
    size_t sValueLen;
    std::string strKey;
    unsigned long ulTimeStamp;
    
    SimpleKVRecord() = default;
    SimpleKVRecord(size_t alValueOffset, size_t asValueLen, std::string const& astrKey) : 
        lValueOffset(alValueOffset), sValueLen(asValueLen), strKey(astrKey) {}
    SimpleKVRecord(size_t alValueOffset, size_t asValueLen, std::string const& astrKey, unsigned long aTimeStamp) : 
        lValueOffset(alValueOffset), sValueLen(asValueLen), strKey(astrKey), ulTimeStamp(aTimeStamp) {}
};


class SimpleKVIndex {
public:
    SimpleKVIndex() : m_ulKeyCount(0), m_iBuckets(256), m_vecMutex(m_iBuckets), m_vecIndex(m_iBuckets) {}
    int get(std::string const& aKey, SimpleKVRecord& outRecord);
    int set(SimpleKVRecord const& aRecord);
    int remove(SimpleKVRecord const& aRecord);
    int mergeIndex(SimpleKVIndex const& aIndex);

    int bucketNum() {
        return m_iBuckets;
    }
    
    std::unordered_map<std::string, SimpleKVRecord> snapshot() {
        std::unordered_map<std::string, SimpleKVRecord> mSnapshot;
        for (int i = 0; i < m_iBuckets; ++i) {
            std::unique_lock<std::mutex> lock(m_vecMutex[i]);
            mSnapshot.insert(m_vecIndex[i].begin(), m_vecIndex[i].end());
        }
        return mSnapshot;
    }

    int keyCount();
private:
    std::atomic<unsigned long> m_ulKeyCount;
    const int m_iBuckets;
    mutable std::vector<std::mutex> m_vecMutex;
    std::vector<std::unordered_map<std::string, SimpleKVRecord>> m_vecIndex;
};

class SimpleKVStore {
public:
    SimpleKVStore() : m_ulTimeStamp(0) {}
    int initStore(const char* aPath, SimpleKVIndex& aIndex);
    int loadIndexFromHint(SimpleKVIndex& aIndex);
    int loadIndex(SimpleKVIndex& aIndex, std::string const& aFileName, int aFileId);
    int loadIndex(SimpleKVIndex& aIndex, std::unique_ptr<File> const& pSource, bool bLoadFromHint, int aFileId);
    int dumpIndexToHint(SimpleKVIndex& aIndex, std::unique_ptr<File>& pHintFile);
    int dumpIndex(SimpleKVIndex& aIndex, std::string const& aDestination);
    
    int compact();
    int compact(std::unique_ptr<File> const& aDataFile);
    int write(int aFd, void* aBuf, size_t aBytes, off_t aOffset);
    int read(int aFd, void* aBuf, size_t aBytes, off_t aOffset);
    int activateNewFile();
    SimpleKVRecord append(std::string const& aKey, std::string const& aValue);
    SimpleKVRecord append(std::string const& aKey, std::string const& aValue, std::unique_ptr<File>& pFile);
    SimpleKVRecord append(std::string const& aKey, std::string const& aValue, std::unique_ptr<File>& pFile, unsigned long pTimeStamp);
    SimpleKVRecord remove(std::string const& aKey);

    size_t fileSize() {
        return m_mapFiles[m_iActiveFileId]->writeOffset();
    }

    std::string getValue(SimpleKVRecord const& aRecord, std::unique_ptr<File> const& pDataFile);
    std::string getValue(SimpleKVRecord const& aRecord);
private:
    pthread_rwlock_t m_mActiveFile = PTHREAD_RWLOCK_INITIALIZER;
    std::atomic<unsigned long> m_ulTimeStamp;
    std::atomic<int> m_iActiveFileId;
    std::string m_strBasePath;
    std::unordered_map<int, std::unique_ptr<File>> m_mapFiles;
    std::unique_ptr<File> m_pActiveFile;
    std::mutex m_mFilesMapMutex;
    

    static const size_t MAX_SINGLE_DATA_FILE_SIZE;
};


class SimpleKVSvr {
public:
    typedef std::function<std::string (std::vector<std::string> const&)> RequestCallbackFn;
    SimpleKVSvr(std::unique_ptr<Cache>&& apCache) : m_oCache(std::move(apCache)), m_cntHit(0), m_cntMiss(0), m_iConnId(0) {}
    void init(const char* path) {
        m_oStore.initStore(path, m_oIndex);
        registerRequestCallback("get", std::bind(&SimpleKVSvr::get, this, std::placeholders::_1));
        registerRequestCallback("set", std::bind(&SimpleKVSvr::set, this, std::placeholders::_1));
        registerRequestCallback("delete", std::bind(&SimpleKVSvr::remove, this, std::placeholders::_1));
        registerRequestCallback("stats", std::bind(&SimpleKVSvr::stats, this, std::placeholders::_1));
        registerRequestCallback("compact", std::bind(&SimpleKVSvr::compact, this, std::placeholders::_1));
    }

    void loop();

    void registerRequestCallback(std::string const& aFuncName, RequestCallbackFn aFunc) {
        m_mapOperations[aFuncName] = aFunc;
    }

    void closeConnectionCallback(std::shared_ptr<TcpConnection> const& apConn);

    void newConnectionCallback(EventLoopThreadPool& oPool, int aSockFd, const struct sockaddr_storage aClientAddr);

    void messageCallback(std::shared_ptr<TcpConnection> const& apConn);

    std::string get(std::vector<std::string> const& args);

    std::string remove(std::vector<std::string> const& args);

    std::string stats(std::vector<std::string> const&);

    std::string set(std::vector<std::string> const& args);

    std::string compact(std::vector<std::string> const&);

    std::string statsString();
private:
    SimpleKVStore m_oStore;
    SimpleKVIndex m_oIndex;
    std::unique_ptr<Cache> m_oCache;
    std::atomic<int> m_cntHit;
    std::atomic<int> m_cntMiss;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> m_mapConnections;
    
    std::unordered_map<std::string, RequestCallbackFn>
        m_mapOperations;
    int m_iConnId;
    EventLoop* m_pLoop;
};