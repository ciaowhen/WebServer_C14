//
// Created by user on 2023/1/16.
//

#include "epoller.h"

Epoller::Epoller(int max_event):m_epoll_fd(epoll_creat(512)), m_events(max_event)
{
    assert(m_epoll_fd >= 0 && m_events.size() > 0);
}

Epoller::~Epoller()
{
    close(m_epoll_fd);
}

bool Epoller::AddFd(int fd, uint32_t events)
{
    if(fd < 0)
    {
        return false;
    }

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events)
{
    if(fd < 0)
    {
        return false;
    }

    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd)
{
    if(fd < 0)
    {
        return false;
    }

    epoll_event ev = {0};
    return 0 == epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeout_ms)
{
    return epoll_wait(m_epoll_fd, &m_events[0], static_cast<int>(m_events.size()), timeout_ms);
}

int Epoller::GetEventFd(size_t i) const
{
    assert(i < m_events.size() && i >= 0);
    return m_events[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const
{
    assert(i < m_events.size() && i >= 0);
    return m_events[i].events;
}



