#pragma once
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <thread>
#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include "Socket.h"
#include "EventLoopThreadPool.h"
#include <unordered_map>
#include <condition_variable>
#include "TcpConnection.h"
#include <chrono>
#include <fstream>

class SimpleKVBenchmark {
public:
    SimpleKVBenchmark(const int aThreadCount, const int aIOThreadCount, const int aOperationsPerThread, const int aKeySize, const int aValueSize) :
        m_iThreadCount(aThreadCount), m_iIOThreadCount(aIOThreadCount), m_iOperationsPerThread(aOperationsPerThread),
        m_iKeySize(aKeySize), m_iValueSize(aValueSize), m_vecTestKVs(aThreadCount), m_vecMapReply(m_iThreadCount) {}
    
    void runTestWrite();

    void runTestRead();
    
private:
    const int m_iThreadCount, m_iIOThreadCount, m_iOperationsPerThread, m_iKeySize, m_iValueSize;
    std::vector<std::unordered_map<std::string, std::string>> m_vecTestKVs;
    std::vector<std::unordered_map<std::string, std::string>> m_vecMapReply; 

    static void randomString(char* s, const int len);

    static void generateTestKV(std::unordered_map<std::string, std::string>& mapTestKV, const int aCount, const int aKeySize, const int aValueSize);

    void loadTestData();

    void generateTestData();

    static void dumpKVToFile(std::unordered_map<std::string, std::string> const& aKV, std::string const& aPath);

    static void loadKVFromFile(std::unordered_map<std::string, std::string>& aKV, std::string const& aPath);

    void verifyReplies();
    
    void testWriteThread(EventLoop* pLoop, const int iThreadIdx);
    
    void testReadThread(EventLoop* pLoop, const int iThreadIdx);

    static void addSetCommand(Buffer& aBuffer, std::string const& aKey, std::string const& aValue);

    static void addGetCommand(Buffer& aBuffer, std::string const& aKey);
};