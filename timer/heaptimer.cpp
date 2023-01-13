//
// Created by user on 2023/1/13.
//

#include "heaptimer.h"

void HeapTimer::OnAdd(int id, int time_out, const TimeOutCallBack &cb)
{
    assert(id >= 0);
    size_t i;
    if(m_ref.count(id) == 0)
    {
        i = m_timer_heap.size();
        m_ref[id] = i;
        m_timer_heap.push_back({id, Clock::now() + MS(time_out), cb});
    }
    else
    {
        i = m_ref[id];
        m_timer_heap[i].expires = Clock::now() + MS(time_out);
        m_timer_heap[i].cb = cb;
        if(!SiftDown(i, m_timer_heap.size()))
        {
            SiftUp(i);
        }
    }
}

void HeapTimer::OnDel(size_t index)
{
    assert(index >= 0 && index < m_timer_heap.size() && !m_timer_heap.empty());
    size_t i = index;
    size_t n = m_timer_heap.size() - 1;
    assert(i <= n);
    if(i < n)
    {
        SwapNode(i, n);
        if(!SiftDown(i, n))
        {
            SiftUp(i);
        }
    }

    m_ref.erase(m_timer_heap.back().id);
    m_timer_heap.pop_back();
}

void HeapTimer::OnAdjust(int id, int timeout)
{
    assert(!m_timer_heap.empty() && m_ref.count(id) > 0);
    m_timer_heap[m_ref[id]].expires = Clock::now() + MS(timeout);
    SiftDown(m_ref[id], m_timer_heap.size());
}

void HeapTimer::OnDoWork(int id)
{
    if(m_timer_heap.empty() || m_ref.count(id) == 0)
    {
        return;
    }

    size_t i = m_ref[id];
    TimerNode node = m_timer_heap[i];
    node.cb();
    OnDel(i);
}

void HeapTimer::OnTick()
{
    if(m_timer_heap.empty())
    {
        return;
    }
    while (!m_timer_heap.empty())
    {
        TimerNode node = m_timer_heap.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
        {
            break;
        }

        node.cb();
        OnPop();
    }
}

void HeapTimer::OnPop()
{
    assert(!m_timer_heap.empty());
    OnDel(0);
}

void HeapTimer::OnClear()
{
    m_ref.clear();
    m_timer_heap.clear();
}

int HeapTimer::GetNextTick()
{
    OnTick();
    size_t res = -1;
    if(!m_timer_heap.empty())
    {
        res = std::chrono::duration_cast<MS>(m_timer_heap.front().expires - Clock::now()).count();
        if(res < 0)
        {
            res = 0;
        }
    }

    return res;
}

void HeapTimer::SiftUp(size_t i)
{
    assert(i >= 0 && i < m_timer_heap.size());
    size_t j = (i - 1) / 2;
    while(j >= 0)
    {
        if (m_timer_heap[j] < m_timer_heap[i])
        {
            break;
        }

        SwapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::SiftDown(size_t index, size_t n)
{
    assert(index >= 0 && index < m_timer_heap.size());
    assert(n >= 0 && n <= m_timer_heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while (j < n)
    {
        if(j + 1 < n && m_timer_heap[j + 1] < m_timer_heap[j])
        {
            ++j;
        }

        if(m_timer_heap[i] < m_timer_heap[j])
        {
            break;
        }

        SwapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }

    return i > index;
}

void HeapTimer::SwapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < m_timer_heap.size());
    assert(j >= 0 && j < m_timer_heap.size());
    std::swap(m_timer_heap[i], m_timer_heap[j]);
    m_ref[m_timer_heap[i].id] = i;
    m_ref[m_timer_heap[j].id] = j;
}