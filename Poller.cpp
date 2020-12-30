#include "Poller.h"
#include <iostream>
#include <cassert>
#include <sstream>
const int Poller::kAdded = 1;
const int Poller::kDeleted = 2;
const int Poller::kNew = -1;

Poller::Poller(EventLoop* aLoop) : m_iPollFd(epoll_create1(EPOLL_CLOEXEC)), m_pLoop(aLoop), m_vecEvents(16) {
    if (m_iPollFd < 0) {
        perror("epoll_create");
        abort();
    }
}

Poller::~Poller() {
    close(m_iPollFd);
}

void Poller::poll(int aTimeoutMs, std::vector<Channel*>& aActiveChannels) {
    int numEvents = ::epoll_wait(m_iPollFd, &*m_vecEvents.begin(), m_vecEvents.size(), aTimeoutMs);
    // int savedErrno = errno;
    if (numEvents > 0) {
        fillActiveChannels(numEvents, aActiveChannels);
        if ((std::size_t)numEvents == m_vecEvents.size()) {
            m_vecEvents.resize(m_vecEvents.size() * 2);
        }
    } else if (numEvents < 0) {
        perror("epoll_wait");
    }
}

void Poller::fillActiveChannels(int aNumEvents, std::vector<Channel*>& aActiveChannels) {
    for (int i = 0; i < aNumEvents; ++i) {
        Channel* channel = static_cast<Channel*>(m_vecEvents[i].data.ptr);
        channel->setReceivedEvents(m_vecEvents[i].events);
        aActiveChannels.push_back(channel);
    }
}

std::string eventsToString(int ev) {
  std::ostringstream oss;
  if (ev & EPOLLIN)
    oss << "IN ";
  if (ev & EPOLLPRI)
    oss << "PRI ";
  if (ev & EPOLLOUT)
    oss << "OUT ";
  if (ev & EPOLLHUP)
    oss << "HUP ";
  if (ev & EPOLLRDHUP)
    oss << "RDHUP ";
  if (ev & EPOLLERR)
    oss << "ERR ";

  return oss.str();
}

void Poller::update(int aOperation, Channel* apChannel) {
    struct epoll_event stEvent;
    bzero(&stEvent, sizeof(stEvent));
    stEvent.data.ptr = apChannel;
    stEvent.events = apChannel->getEvents();
    int iRet = 0;
    // char buffer[1024] = {0};
    // std::string events = eventsToString(channel->getEvents());
    // snprintf(buffer, 1024, "epoll_ctl (tid %d) operation %d on fd %d events %s\n", CurrentThread::tid(), operation, channel->getFd(), events.c_str());
    // write(STDERR_FILENO, buffer, strlen(buffer));
    if ((iRet = ::epoll_ctl(m_iPollFd, aOperation, apChannel->getFd(), &stEvent))) {
        std::cerr << "epoll_ctl error: " << aOperation << " " << apChannel->getFd() << " " << iRet << std::endl;
        perror("epoll_ctl");
        abort();
    }
}

void Poller::updateChannel(Channel* apChannel) {
    m_pLoop->assertInLoopThread();
    int iIndex = apChannel->getIndex();
    if (iIndex == kNew || iIndex == kDeleted) {
        int iFd = apChannel->getFd();
        if (iIndex == kNew) {
            // assert(!m_mapChannels.count(iFd));
            m_mapChannels[iFd] = apChannel;
        } else {
            // assert(m_mapChannels.count(iFd));
        }
        apChannel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, apChannel);
    } else {
        if (apChannel->isNoneEvents()) {
            update(EPOLL_CTL_DEL, apChannel);
            apChannel->setIndex(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, apChannel);
        }
    }
}

void Poller::removeChannel(Channel* apChannel) {
    m_pLoop->assertInLoopThread();
    int iFd = apChannel->getFd();
    int iIndex = apChannel->getIndex();
    assert(iIndex == kAdded || iIndex == kDeleted);
    m_mapChannels.erase(iFd);
    if (iIndex == kAdded) {
        update(EPOLL_CTL_DEL, apChannel);
    }
    apChannel->setIndex(kNew);
}