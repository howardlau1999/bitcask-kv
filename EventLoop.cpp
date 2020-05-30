#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include <vector>
#include <iostream>
#include <cassert>
#include <algorithm>
#include "CurrentThread.h"

int createEventfd() {
    int iEventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (iEventFd < 0) {
        std::cerr << "Failed in eventfd" << std::endl;
        perror("eventfd");
        exit(1);
    }
    return iEventFd;
}

EventLoop::EventLoop() : 
    m_pPoller(new Poller(this)), m_bQuit(false), m_bLooping(false), m_iThreadId(CurrentThread::getTid()),
    m_iEventFd(createEventfd()), m_pWakeupChannel(new Channel(this, m_iEventFd)) {
    m_pWakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    m_pWakeupChannel->enableReading();
}

bool EventLoop::isInLoopThread() {
    return m_iThreadId == CurrentThread::getTid();
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->getEventLoop() == this);
    assertInLoopThread();
    m_pPoller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->getEventLoop() == this);
    assertInLoopThread();
    if (m_bHandlingEvents) {
        assert(m_pCurrentActiveChannel == channel || std::find(m_vecActiveChannels.begin(), m_vecActiveChannels.end(), channel) == m_vecActiveChannels.end());
    }
    m_pPoller->removeChannel(channel);
}

void EventLoop::loop() {
    assertInLoopThread();
    m_bQuit = false;
    m_bLooping = true;
    while (!m_bQuit) {
        m_vecActiveChannels.clear();
        m_pPoller->poll(1000, m_vecActiveChannels);
        m_bHandlingEvents = true;
        for (auto channel : m_vecActiveChannels) {
            m_pCurrentActiveChannel = channel;
            m_pCurrentActiveChannel->handleEvent();
        }
        m_bHandlingEvents = false;
        m_pCurrentActiveChannel = nullptr;
        doPendingFunctors();
    }
    m_bLooping = false;
}

void EventLoop::runInLoop(std::function<void()> aFunc) {
    if (isInLoopThread()) { 
        aFunc();
    } else {
        queueInLoop(std::move(aFunc));
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<std::function<void()>> functors;
    m_bCallingPendingFunctors = true;

    {
        std::unique_lock<std::mutex> lock(m_mMutex);
        functors.swap(m_vecPendingFunctors);
    }

    for (auto const& functor : functors) {
        functor();
    }
    
    m_bCallingPendingFunctors = false;
}
void EventLoop::queueInLoop(std::function<void()> cb) {
    {
        std::unique_lock<std::mutex> lock(m_mMutex);
        m_vecPendingFunctors.push_back(std::move(cb));
    }

    if (!isInLoopThread() || m_bCallingPendingFunctors) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t ulOne = 1;
    ssize_t n = ::write(m_iEventFd, &ulOne, sizeof(ulOne));
}

void EventLoop::handleRead() {
    uint64_t ulOne = 1;
    ssize_t n = ::read(m_iEventFd, &ulOne, sizeof(ulOne));
}

void EventLoop::quit() {
    m_bQuit = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

EventLoop::~EventLoop() {
    m_pWakeupChannel->disableAll();
    m_pWakeupChannel->remove();
    ::close(m_iEventFd);
    m_bQuit = true;
}