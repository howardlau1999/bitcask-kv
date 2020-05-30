#include "SimpleKVSvr.h"
#include "Protocol.h"
#include "LRUCache.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include <iostream>
#include <unistd.h>
#include "Utils.h"
#include "Encoding.h"
#include <thread>
#include "CurrentThread.h"

const size_t SimpleKVStore::MAX_SINGLE_DATA_FILE_SIZE = 2 << 30; // 1GB


int SimpleKVIndex::keyCount() {
    return m_ulKeyCount.load();
}

int SimpleKVIndex::get(std::string const& aKey, SimpleKVRecord& outRecord) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mIndex = m_vecIndex[iBucketIdx];

    if (mIndex.count(aKey) == 0) {
        return -1;
    }
    outRecord = mIndex[aKey];
    
    if (outRecord.sValueLen == 0) {
        return -1;
    }
    return 0;
}

int SimpleKVIndex::set(SimpleKVRecord const& aRecord) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aRecord.strKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mIndex = m_vecIndex[iBucketIdx];

    if (mIndex.count(aRecord.strKey)) {
        if (mIndex[aRecord.strKey].ulTimeStamp < aRecord.ulTimeStamp) {
            mIndex[aRecord.strKey] = aRecord;
        }
    } else {
        mIndex[aRecord.strKey] = aRecord;
        m_ulKeyCount.fetch_add(1);
    }

    return 0;
}

int SimpleKVIndex::remove(SimpleKVRecord const& aRecord) {
    const int iBucketIdx = utils::getBucketIndex(m_iBuckets, aRecord.strKey);
    std::unique_lock<std::mutex> lock(m_vecMutex[iBucketIdx]);
    auto& mIndex = m_vecIndex[iBucketIdx];

    if (mIndex.count(aRecord.strKey)) {
        if (mIndex[aRecord.strKey].ulTimeStamp < aRecord.ulTimeStamp) {
            mIndex[aRecord.strKey] = aRecord;
            m_ulKeyCount.fetch_sub(1);
        }
    }

    return 0;
}

int SimpleKVIndex::mergeIndex(SimpleKVIndex const& aIndex) {
    for (int i = 0; i < aIndex.m_iBuckets; ++i) {
        std::unique_lock<std::mutex> lock_other(aIndex.m_vecMutex[i]);
        std::unique_lock<std::mutex> lock_self(m_vecMutex[i]);
        for (auto const& pair : aIndex.m_vecIndex[i]) {
            auto const& stRecord = pair.second;
            if (m_vecIndex[i].count(stRecord.strKey)) {
                if (m_vecIndex[i][stRecord.strKey].ulTimeStamp < stRecord.ulTimeStamp) {
                    m_vecIndex[i][stRecord.strKey] = stRecord;
                }
            } else {
                m_vecIndex[i][stRecord.strKey] = stRecord;
                m_ulKeyCount.fetch_add(1);
            }
        }
    }
   
    return 0;
}

int SimpleKVStore::loadIndex(SimpleKVIndex& aIndex, std::string const& aFileName, int aFileId) {
    if (!utils::haveReadAccess(aFileName)) {
        return -1;
    }
    
    struct stat stStat;
    std::unique_ptr<File> pFile(new File(aFileName.c_str(), O_RDONLY | O_CREAT | O_CLOEXEC | O_APPEND, 0));

    std::string strHintPath = std::string(aFileName) + ".hint";
    std::unique_ptr<File> pHintFile;

    if (stat(strHintPath.c_str(), &stStat)) {
        pHintFile = nullptr;
    } else {
        pHintFile.reset(new File(strHintPath.c_str(), O_RDONLY | O_CLOEXEC | O_APPEND, stStat.st_size));
    }

    if (pHintFile) {
        loadIndex(aIndex, pHintFile, true, aFileId);
    } else {
        loadIndex(aIndex, pFile, false, aFileId);
    }

    {
        std::unique_lock<std::mutex> lock(m_mFilesMapMutex);
        m_mapFiles.insert(std::make_pair(aFileId, std::move(pFile)));
    }
    return 0;
}

int SimpleKVStore::initStore(const char* aPath, SimpleKVIndex& aIndex) {
    m_strBasePath = aPath;
    // Check Compact First
    loadIndex(aIndex, m_strBasePath + ".compact", -1);
    std::vector<std::thread> vecLoadingThreads;
    for (int i = 0; i < 2048; ++i) {
        std::string strCurFilePath = m_strBasePath + std::to_string(i);
        struct stat stStat;

        if (stat(strCurFilePath.c_str(), &stStat)) {
            stStat.st_size = 0;
            m_iActiveFileId = i;
            std::unique_ptr<File> pActiveFile(new File(strCurFilePath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_APPEND, stStat.st_size));
            {
                std::unique_lock<std::mutex> lock(m_mFilesMapMutex);
                m_mapFiles.insert(std::make_pair(m_iActiveFileId.load(), std::move(pActiveFile)));
            }
            break;
        }

        // Load Parallelly Using Multiple Threads
        vecLoadingThreads.push_back(std::thread([i, &aIndex, this]() {
            std::cout << "Load " << m_strBasePath + std::to_string(i) << std::endl;
            this->loadIndex(aIndex, m_strBasePath + std::to_string(i), i);
        }));
    }

    for (auto& thread : vecLoadingThreads) {
        thread.join();
    }
    
    return 0;
}

int SimpleKVStore::dumpIndexToHint(SimpleKVIndex& aIndex, std::unique_ptr<File>& pHintFile) {
    std::string strHintPath = m_strBasePath + ".hint";
    pHintFile.reset(new File(strHintPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_APPEND, 0));
    auto const& mapSnapshot = aIndex.snapshot();
    for (auto const& pair : mapSnapshot) {
        if (pair.second.sValueLen == 0) {
            continue;
        }
        std::vector<char> vecEncoded = Encoding::encodeIndex(pair.first, pair.second.sValueLen, pair.second.lValueOffset, pair.second.ulTimeStamp);
        off_t dummy;
        pHintFile->append(&vecEncoded[0], vecEncoded.size(), dummy);
    }
    return 0;
}

int SimpleKVStore::dumpIndex(SimpleKVIndex& aIndex, std::string const& aDestination) {

    return 0;
}

int SimpleKVStore::activateNewFile() {
    pthread_rwlock_wrlock(&m_mActiveFile);
    m_mapFiles[m_iActiveFileId]->disableWriting();
    ++m_iActiveFileId;
    std::string strNewFilePath = m_strBasePath + std::to_string(m_iActiveFileId);
    std::unique_ptr<File> pNewFile(new File(strNewFilePath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_APPEND, 0));
    m_mapFiles.insert(std::make_pair<int, std::unique_ptr<File>>(m_iActiveFileId.load(), std::move(pNewFile)));
    pthread_rwlock_unlock(&m_mActiveFile);
    return 0;
}

int SimpleKVStore::compact() {
    compact(m_pActiveFile);
    return 0;
}
int SimpleKVStore::compact(std::unique_ptr<File> const& aDataFile) { 
    std::string strHintFileName = m_strBasePath + ".compact.hint";
    std::string strNewFileName = m_strBasePath + ".compact";
    std::unique_ptr<File> pNewFile(new File(strNewFileName.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_APPEND, 0));
    std::unique_ptr<File> pHintFile(new File(strHintFileName.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_APPEND, 0));
    pHintFile->truncate();
    pNewFile->truncate();
    SimpleKVIndex oTmpIndex;
    for (auto const& filePair : m_mapFiles) {
        if (!filePair.second->readOnly()) {
            continue;
        }
        SimpleKVIndex oTmpPartialIndex;
        loadIndex(oTmpPartialIndex, filePair.second, false, filePair.first);
        oTmpIndex.mergeIndex(oTmpPartialIndex);
    }
    auto const& mapSnapshot = oTmpIndex.snapshot();
    for (auto const& pair : mapSnapshot) {
        if (pair.second.sValueLen == 0) {
            continue;
        }
        SimpleKVRecord stNewRecord = this->append(pair.first, getValue(pair.second), pNewFile, pair.second.ulTimeStamp);
        std::vector<char> vecEncoded = Encoding::encodeIndex(pair.first, stNewRecord.sValueLen, stNewRecord.lValueOffset, stNewRecord.ulTimeStamp);
        off_t dummy;
        pHintFile->append(&vecEncoded[0], vecEncoded.size(), dummy);
    }
    return 0;
}

int SimpleKVStore::loadIndex(SimpleKVIndex& aIndex, std::unique_ptr<File> const& pSource, bool bLoadFromHint, int aFileId) {
    off_t lOffset = 0;
    while (true) {
        size_t sKeyLen = 0;
        size_t sValueLen = 0;
        unsigned long ulTimeStamp = 0;

        if (pSource->read(&ulTimeStamp, sizeof(ulTimeStamp), lOffset) < sizeof(ulTimeStamp)) {
            break;
        }

        if (ulTimeStamp > m_ulTimeStamp.load()) {
            m_ulTimeStamp.store(ulTimeStamp);
        }

        lOffset += sizeof(ulTimeStamp);
        
        if (pSource->read(&sKeyLen, sizeof(sKeyLen), lOffset) < sizeof(sKeyLen)) {
            break;
        }

        lOffset += sizeof(sKeyLen);

        std::string strKey;
        strKey.resize(sKeyLen);

        if (pSource->read(&strKey[0], sKeyLen, lOffset) < sKeyLen) {
            break;
        }

        lOffset += sKeyLen;

        if (pSource->read(&sValueLen, sizeof(sValueLen), lOffset) < sizeof(sValueLen)) {
            break;
        }

        lOffset += sizeof(sValueLen);

        if (sValueLen == 0) {
            SimpleKVRecord tRecord(0, 0, strKey, ulTimeStamp);
            aIndex.remove(tRecord);
        } else {
            if (bLoadFromHint) {
                off_t lValueOffset;

                if (pSource->read(&lValueOffset, sizeof(lValueOffset), lOffset) < sizeof(lValueOffset)) {
                    break;
                } else {
                    lOffset += sizeof(lValueOffset);

                    SimpleKVRecord tRecord(lValueOffset, sValueLen, strKey, ulTimeStamp);
                    tRecord.iFileId = aFileId;
                    aIndex.set(tRecord);
                }
            } else {
                SimpleKVRecord tRecord(lOffset, sValueLen, strKey, ulTimeStamp);
                tRecord.iFileId = aFileId;
                aIndex.set(tRecord);
                lOffset += sValueLen;
            }
        }
    }
    return 0;
}

SimpleKVRecord SimpleKVStore::append(std::string const& aKey, std::string const& aValue, std::unique_ptr<File>& pFile, unsigned long ulTimeStamp) {
    std::vector<char> vecEncodedRecord = Encoding::encodeRecord(aKey, aValue, ulTimeStamp);
    off_t lRecordOffset;
    pFile->append(&vecEncodedRecord[0], vecEncodedRecord.size(), lRecordOffset);
    off_t lValueOffset = lRecordOffset + sizeof(ulTimeStamp) + aKey.size() + sizeof(aKey.size()) + sizeof(aValue.size());
    return SimpleKVRecord(lValueOffset, aValue.size(), aKey, ulTimeStamp);
}

SimpleKVRecord SimpleKVStore::append(std::string const& aKey, std::string const& aValue, std::unique_ptr<File>& pFile) {
    m_ulTimeStamp.fetch_add(1);
    unsigned long ulTimeStamp = m_ulTimeStamp.load();
    return append(aKey, aValue, pFile, ulTimeStamp);
}

SimpleKVRecord SimpleKVStore::append(std::string const& aKey, std::string const& aValue) {
    pthread_rwlock_rdlock(&m_mActiveFile);
    if (m_mapFiles[m_iActiveFileId]->writeOffset() >= MAX_SINGLE_DATA_FILE_SIZE) {
        pthread_rwlock_unlock(&m_mActiveFile);
        activateNewFile();
        pthread_rwlock_rdlock(&m_mActiveFile);
    } 
    SimpleKVRecord stRecord = append(aKey, aValue, m_mapFiles[m_iActiveFileId]);
    stRecord.iFileId = m_iActiveFileId;
    pthread_rwlock_unlock(&m_mActiveFile);
    return stRecord;
}

SimpleKVRecord SimpleKVStore::remove(std::string const& aKey) {
    return append(aKey, "");
}

std::string SimpleKVStore::getValue(SimpleKVRecord const& aRecord, std::unique_ptr<File> const& pDataFile) {
    std::string strValue;
    strValue.resize(aRecord.sValueLen);
    pDataFile->read(&strValue[0], aRecord.sValueLen, aRecord.lValueOffset);
    return strValue;
}

std::string SimpleKVStore::getValue(SimpleKVRecord const& aRecord) {
    return getValue(aRecord, m_mapFiles[aRecord.iFileId]);
}

std::string SimpleKVSvr::statsString() {
    int iKeyCount = m_oIndex.keyCount();
    int iFileSize = m_oStore.fileSize();
    int iHit = m_cntHit;
    int iMiss = m_cntMiss;
    int iMemRssPages = -1;
    int iMemVmemPages;
    FILE* pStatm = fopen("/proc/self/statm", "r");
    fscanf(pStatm, "%d%d", &iMemVmemPages, &iMemRssPages);
    fclose(pStatm);
    return "count: " + std::to_string(iKeyCount) + ", mem: " + std::to_string(iMemRssPages * 4) + 
    " KB, file: " + std::to_string(iFileSize) + " B, hits: " + std::to_string(iHit) + 
    ", misses: " + std::to_string(iMiss) + ", cache hits: " + std::to_string(m_oCache->cacheHit()) + 
    ", cache misses: " + std::to_string(m_oCache->cacheMiss());
}

void SimpleKVSvr::loop()  {
    EventLoop oLoop;
    EventLoop *pLoop = &oLoop;
    m_pLoop = pLoop;
    Acceptor *pAcceptor;
    EventLoopThreadPool oPool(4, pLoop);
    oPool.start();

    struct addrinfo hints, *servinfo, *p;
    int connid = 1;
    const int BACKLOG = 10;
    int rv, fd;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo("0.0.0.0", "7777", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        pAcceptor = new Acceptor(pLoop, p);
        break;
    }

    freeaddrinfo(servinfo);

    pAcceptor->setNewConnectionCallback(
        std::bind(&SimpleKVSvr::newConnectionCallback, this, std::ref(oPool), std::placeholders::_1, std::placeholders::_2)
    );

    pAcceptor->listen();
    std::cerr << "Server Listening on 0.0.0.0:7777" << std::endl;
    pLoop->loop();
}

void SimpleKVSvr::newConnectionCallback(EventLoopThreadPool& oPool, int aSockFd, const struct sockaddr_storage aClientAddr) {
    m_pLoop->assertInLoopThread();
    ++m_iConnId;
    EventLoop* pLoop = oPool.getNextLoop();

    char s[INET6_ADDRSTRLEN] = {0};
    inet_ntop(aClientAddr.ss_family,
                getInetAddr((struct sockaddr *)&aClientAddr), s, sizeof s);
    printf("server: thread %d accepted connection %d(sockfd: %d) from %s:%d\n", pLoop->getThreadId(), m_iConnId, aSockFd, s,
            getInetPort((struct sockaddr *)&aClientAddr));

    std::shared_ptr<TcpConnection> pConn = std::make_shared<TcpConnection>(pLoop, aSockFd, m_iConnId);
    m_mapConnections[m_iConnId] = pConn;

    pConn->setMessageCallback(std::bind(&SimpleKVSvr::messageCallback, this, std::placeholders::_1));
    pConn->setCloseCallback([this](std::shared_ptr<TcpConnection> const& apConn) {
        m_pLoop->runInLoop(std::bind(&SimpleKVSvr::closeConnectionCallback, this, apConn));
    });
    pLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, pConn));
}

void SimpleKVSvr::closeConnectionCallback(std::shared_ptr<TcpConnection> const& apConn) {
    m_pLoop->assertInLoopThread();
    printf("server: connection %d disconnected\n", apConn->getConnId());
    EventLoop* ioLoop = apConn->getLoop();
    m_mapConnections.erase(apConn->getConnId());
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, apConn));
    
}

void SimpleKVSvr::messageCallback(std::shared_ptr<TcpConnection> const& apConn) {
    auto& buffer = apConn->getInputBuffer();
    const char* pLF = buffer.findEOL();
    Buffer replyBuffer;
    while ((pLF = buffer.findEOL())) {
        std::string strRequestLine(buffer.peek(), pLF);
        std::vector<std::string> arrRequest;
        bool bClose = false;
        if (Protocol::parseRequestLine(strRequestLine, arrRequest)) {
            std::string reply;

            if (arrRequest[0] == "quit") {
                reply = "OK";
                bClose = true;
            } else if (m_mapOperations.count(arrRequest[0])) {
                reply = m_mapOperations[arrRequest[0]](arrRequest);
            } 
            
            replyBuffer.append(&reply[0], reply.size());
            replyBuffer.append("\n", 1);
            replyBuffer.prepend("+", 1);
            apConn->send(&replyBuffer);
            
            if (bClose) {
                apConn->handleClose();
            }
        }
        buffer.retrieveUntil(pLF + 1);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    SimpleKVSvr oSimpleKVSvr(std::unique_ptr<Cache>(new SimpleKVLRUCache(10000)));
    oSimpleKVSvr.init("kvstore");
    oSimpleKVSvr.loop();
    return 0;
}