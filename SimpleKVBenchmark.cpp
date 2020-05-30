#include "SimpleKVBenchmark.h"

void SimpleKVBenchmark::addSetCommand(Buffer& aBuffer, std::string const& aKey, std::string const& aValue) {
    aBuffer.append("-set ", 5);
    aBuffer.append(&*aKey.begin(), aKey.size());
    aBuffer.append(" ", 1);
    aBuffer.append(&*aValue.begin(), aValue.size());
    aBuffer.append("\n", 1);
}

void SimpleKVBenchmark::addGetCommand(Buffer& aBuffer, std::string const& aKey) {
    aBuffer.append("-get ", 5);
    aBuffer.append(&*aKey.begin(), aKey.size());
    aBuffer.append("\n", 1);
}

void SimpleKVBenchmark::dumpKVToFile(std::unordered_map<std::string, std::string> const& aKV, std::string const& aPath) {
    std::ofstream ofs(aPath);
    for (auto const& pair : aKV) {
        ofs << pair.first << std::endl;
        ofs << pair.second << std::endl;
    }
}

void SimpleKVBenchmark::generateTestKV(std::unordered_map<std::string, std::string>& mapTestKV, const int aCount, const int aKeySize, const int aValueSize) {
    mapTestKV.clear();
    std::string strKey, strValue;
    strKey.resize(aKeySize);
    strValue.resize(aValueSize);
    for (int i = 0; i < aCount; ++i) {
        SimpleKVBenchmark::randomString(&*strKey.begin(), aKeySize);
        SimpleKVBenchmark::randomString(&*strValue.begin(), aValueSize);
        mapTestKV.insert(std::make_pair(strKey, strValue));
    }
}

void SimpleKVBenchmark::randomString(char* s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
}

void SimpleKVBenchmark::loadTestData() {
    std::cout << "Loading Test Data From Files..." << std::endl;
    
    std::vector<std::thread> vecLoadThreads;
    for (int iThreadIdx = 0; iThreadIdx < m_iThreadCount; ++iThreadIdx) {
        vecLoadThreads.push_back(std::thread(std::bind(&SimpleKVBenchmark::loadKVFromFile, 
            std::ref(m_vecTestKVs[iThreadIdx]),
            "testdata/" + std::to_string(iThreadIdx)
        )));
    }

    for (auto& thread : vecLoadThreads) {
        thread.join();
    }

    std::cout << "Loaded Test Data." << std::endl;
}

void SimpleKVBenchmark::generateTestData() {
    std::cout << "Generating Test Data..." << std::endl;
    
    std::vector<std::thread> vecGenThreads;
    for (int iThreadIdx = 0; iThreadIdx < m_iThreadCount; ++iThreadIdx) {
        vecGenThreads.push_back(std::thread([this, iThreadIdx]() {
            SimpleKVBenchmark::generateTestKV(m_vecTestKVs[iThreadIdx], m_iOperationsPerThread, m_iKeySize, m_iValueSize);
            SimpleKVBenchmark::dumpKVToFile(m_vecTestKVs[iThreadIdx], "testdata/" + std::to_string(iThreadIdx));
        }));
    }

    for (auto& thread : vecGenThreads) {
        thread.join();
    }

    std::cout << "Test Data Generated." << std::endl;
}

void SimpleKVBenchmark::loadKVFromFile(std::unordered_map<std::string, std::string>& aKV, std::string const& aPath) {
    aKV.clear();
    std::ifstream ifs(aPath);
    while (!ifs.eof()) {
        std::string strKey, strValue;
        std::getline(ifs, strKey);
        std::getline(ifs, strValue);
        if (ifs.eof()) break;
        aKV.insert(std::make_pair(strKey, strValue));
    }
}

void SimpleKVBenchmark::verifyReplies() {
    std::cout << "Verifying values..." << std::endl;

    std::vector<std::thread> vecVerifyThreads;
    for (int iThreadIdx = 0; iThreadIdx < m_iThreadCount; ++iThreadIdx) {
        vecVerifyThreads.push_back(std::thread([this, iThreadIdx]() {
            int iMismatched = 0;
            auto const& mapReply = m_vecMapReply[iThreadIdx];
            auto const& mapTestKVs = m_vecTestKVs[iThreadIdx];

            if (mapReply.size() != mapTestKVs.size()) {
                char buffer[10240] = {0};
                snprintf(buffer, sizeof(buffer), "\nThread %d Expected Map Size: %ul\nGot Size: %ul\n", iThreadIdx, mapTestKVs.size(),
                    mapReply.size() 
                );
                write(STDOUT_FILENO, buffer, strlen(buffer));
                return;
            }

            for (auto const& pair : mapReply) {
                if (mapTestKVs.at(pair.first) != pair.second) {
                    char buffer[10240] = {0};
                    snprintf(buffer, sizeof(buffer), "\nThread %d Key: %s\nExpected Value: %s\nGot Value: %s\n", iThreadIdx, pair.first.c_str(),
                        mapTestKVs.at(pair.first).c_str(), pair.second.c_str()
                    );
                    write(STDOUT_FILENO, buffer, strlen(buffer));
                }
            }
        }));
    }

    for (auto& thread : vecVerifyThreads) {
        thread.join();
    }

    std::cout << "Finished verification." << std::endl;
}

void SimpleKVBenchmark::testReadThread(EventLoop* pLoop, const int iThreadIdx) {
    std::unordered_map<std::string, std::string> const& mapTestKV = m_vecTestKVs[iThreadIdx];
    std::mutex mMutex;
    std::condition_variable condFinish;
    int iReplyCount = 0;
    bool bQuit = false;
    int iConnFd = connectTo("127.0.0.1", "7777");
    std::shared_ptr<TcpConnection> pConn(std::make_shared<TcpConnection>(pLoop, iConnFd, iThreadIdx));

    Buffer oSendBuffer;
    auto pIt = mapTestKV.cbegin();
    auto pEnd = mapTestKV.cend();
    pConn->setMessageCallback([&iReplyCount, &mMutex, &condFinish, &mapTestKV, &pIt, &pEnd, &oSendBuffer, iThreadIdx, &bQuit, this](std::shared_ptr<TcpConnection> const& conn) {
        auto& oInputBuffer = conn->getInputBuffer();
        const char *pLF = nullptr;
        while ((pLF = oInputBuffer.findEOL())) {
            ++iReplyCount;
            std::string replyValue(oInputBuffer.peek() + 1, pLF);
            m_vecMapReply[iThreadIdx][pIt->first] = replyValue;

            if (++pIt != pEnd) {
                auto const& strKey = pIt->first;
                SimpleKVBenchmark::addGetCommand(oSendBuffer, strKey);
                conn->send(&oSendBuffer);
            }

            oInputBuffer.retrieveUntil(pLF + 1);
        }

        if (iReplyCount == m_iOperationsPerThread) {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                bQuit = true;
            }
            condFinish.notify_all();
        }
    });

    pLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, pConn));

    auto const& strKey = pIt->first;
    SimpleKVBenchmark::addGetCommand(oSendBuffer, strKey);
    pConn->send(&oSendBuffer);
    
    {
        std::unique_lock<std::mutex> lock(mMutex);
        condFinish.wait_for(lock, std::chrono::seconds(10), [&bQuit](){ return bQuit; });
    }

    if (iReplyCount != m_iOperationsPerThread) {
        char arrBuffer[256] = {0};
        snprintf(arrBuffer, 255, "Thread %d Reply Count %d Is Different From %d\n", iThreadIdx, iReplyCount, m_iOperationsPerThread);
        write(STDOUT_FILENO, arrBuffer, strlen(arrBuffer));
    }
}

void SimpleKVBenchmark::testWriteThread(EventLoop* pLoop, const int iThreadIdx) {
    std::unordered_map<std::string, std::string> const& mapTestKV = m_vecTestKVs[iThreadIdx];
    std::mutex mMutex;
    std::condition_variable condFinish;
    int iReplyCount = 0;
    bool bQuit = false;
    int iConnFd = connectTo("127.0.0.1", "7777");
    std::shared_ptr<TcpConnection> pConn(std::make_shared<TcpConnection>(pLoop, iConnFd, iThreadIdx));
    
    pConn->setCloseCallback([&bQuit, &condFinish, &mMutex](std::shared_ptr<TcpConnection> const& conn) {
        EventLoop* ioLoop = conn->getLoop();
        std::cout << "Connection Closed" << std::endl;
        ioLoop->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
        {
            std::unique_lock<std::mutex> lock(mMutex);
            bQuit = true;
        }
        condFinish.notify_all();
    });

    Buffer oSendBuffer;
    auto pSetIt = mapTestKV.cbegin();
    auto pEnd = mapTestKV.cend();

    pConn->setMessageCallback([&iReplyCount, &mMutex, &condFinish, &mapTestKV, &pSetIt, &pEnd, &oSendBuffer, iThreadIdx, &bQuit, this](std::shared_ptr<TcpConnection> const& conn) {
        auto& oInputBuffer = conn->getInputBuffer();
        const char *pLF = nullptr;
        while ((pLF = oInputBuffer.findEOL())) {
            ++iReplyCount;
            
            if (++pSetIt != pEnd) {
                auto const& strKey = pSetIt->first;
                auto const& strValue = pSetIt->second;
                SimpleKVBenchmark::addSetCommand(oSendBuffer, strKey, strValue);
                conn->send(&oSendBuffer);
            }

            oInputBuffer.retrieveUntil(pLF + 1);
        }

        if (iReplyCount == m_iOperationsPerThread) {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                bQuit = true;
            }
            condFinish.notify_all();
        }
    });

    pLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, pConn));

    auto const& strKey = pSetIt->first;
    auto const& strValue = pSetIt->second;
    SimpleKVBenchmark::addSetCommand(oSendBuffer, strKey, strValue);
    pConn->send(&oSendBuffer);
    

    {
        std::unique_lock<std::mutex> lock(mMutex);
        condFinish.wait_for(lock, std::chrono::seconds(10), [&bQuit](){ return bQuit; });
    }

    if (iReplyCount != m_iOperationsPerThread) {
        char arrBuffer[256] = {0};
        snprintf(arrBuffer, 255, "Thread %d Reply Count %d Is Different From %d\n", iThreadIdx, iReplyCount, m_iOperationsPerThread);
        write(STDOUT_FILENO, arrBuffer, strlen(arrBuffer));
    }
    
}
void SimpleKVBenchmark::runTestWrite() {
    generateTestData();
    
    EventLoop baseLoop;
    EventLoopThreadPool oPool(m_iIOThreadCount, &baseLoop);
    oPool.start();
    std::cout << "Started EventLoop Pool." << std::endl;
    
    struct timespec stStart, stFinish;
    double lfElapsedTime;
    clock_gettime(CLOCK_MONOTONIC, &stStart);
    
    std::cout << "Begin Testing..." << std::endl;
    std::vector<std::thread> vecTestThreads;
    for (int iThreadIdx = 0; iThreadIdx < m_iThreadCount; ++iThreadIdx) {
        EventLoop* pLoop = oPool.getNextLoop();
        vecTestThreads.push_back(std::thread(std::bind(&SimpleKVBenchmark::testWriteThread, this, pLoop, iThreadIdx)));
    }
    
    for (auto& thread : vecTestThreads) {
        thread.join();
    }

    std::cout << "Finished." << std::endl;

    clock_gettime(CLOCK_MONOTONIC, &stFinish);
    lfElapsedTime = (stFinish.tv_sec - stStart.tv_sec) * 1000000000.0 +
                  (stFinish.tv_nsec - stStart.tv_nsec);
    
    printf("Used time: %fs\n", lfElapsedTime / 1000000000.0);
    printf("Operations: %d\n", m_iThreadCount * m_iOperationsPerThread);
    printf("QPS: %f\n", (double)(m_iThreadCount * m_iOperationsPerThread) / lfElapsedTime * 1000000000.0);
}

void SimpleKVBenchmark::runTestRead() {
    loadTestData();

    EventLoop baseLoop;
    EventLoopThreadPool oPool(m_iIOThreadCount, &baseLoop);
    oPool.start();
    std::cout << "Started EventLoop Pool." << std::endl;
    
    struct timespec stStart, stFinish;
    double lfElapsedTime;
    clock_gettime(CLOCK_MONOTONIC, &stStart);
    
    std::cout << "Begin Testing..." << std::endl;
    std::vector<std::thread> vecTestThreads;
    for (int iThreadIdx = 0; iThreadIdx < m_iThreadCount; ++iThreadIdx) {
        EventLoop* pLoop = oPool.getNextLoop();
        vecTestThreads.push_back(std::thread(std::bind(&SimpleKVBenchmark::testReadThread, this, pLoop, iThreadIdx)));
    }
    
    for (auto& thread : vecTestThreads) {
        thread.join();
    }

    std::cout << "Finished." << std::endl;

    verifyReplies();

    clock_gettime(CLOCK_MONOTONIC, &stFinish);
    lfElapsedTime = (stFinish.tv_sec - stStart.tv_sec) * 1000000000.0 +
                  (stFinish.tv_nsec - stStart.tv_nsec);
    
    printf("Used time: %fs\n", lfElapsedTime / 1000000000.0);
    printf("Operations: %d\n", m_iThreadCount * m_iOperationsPerThread);
    printf("QPS: %f\n", (double)(m_iThreadCount * m_iOperationsPerThread) / lfElapsedTime * 1000000000.0);
}


int main(int argc, char *argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s threadCount operationsPerTheard keySize(byte) valueSize(byte) r/w\n", argv[0]);
        exit(1);
    }

    int iThreadCount = atoi(argv[1]);
    int iOperationsPerThread = atoi(argv[2]);
    const int iKeySize = atoi(argv[3]);
    const int iValueSize = atoi(argv[4]);
    srand(100);

    SimpleKVBenchmark oBenchmark(iThreadCount, 4, iOperationsPerThread, iKeySize, iValueSize);
    if (argv[5][0] == 'w') {
        oBenchmark.runTestWrite();
    } else {
        oBenchmark.runTestRead();
    }
    
    return 0;
}