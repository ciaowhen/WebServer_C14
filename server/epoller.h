//
// Created by user on 2023/1/16.
//

#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <errno.h>
#include <cstdint>

class Epoller
{
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int timeout_ms = -1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;

private:
    int m_epoll_fd;
    std::vector<struct epoll_event> m_events;
};

#endif //EPOLLER_H
