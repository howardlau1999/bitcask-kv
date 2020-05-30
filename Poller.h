#pragma once
#include "Channel.h"
#include "Socket.h"
#include "EventLoop.h"
#include <vector>
#include <unordered_map>
class Poller {

public:
    static const int kAdded, kNew, kDeleted;
    Poller(EventLoop* aLoop);
    ~Poller();
    void poll(int aTimeoutMs, std::vector<Channel*>& aActiveChannels);
    void update(int aOperation, Channel* apChannel);
    void updateChannel(Channel* apChannel);
    void removeChannel(Channel* apChannel);
    void fillActiveChannels(int aNumEvents, std::vector<Channel*>& aActiveChannels);

private:
    int m_iPollFd;
    EventLoop* m_pLoop;
    std::unordered_map<int, Channel*> m_mapChannels;
    std::vector<struct epoll_event> m_vecEvents;
};