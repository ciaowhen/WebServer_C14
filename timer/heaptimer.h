//
// Created by user on 2023/1/13.
//

#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>
#include "../log/log.h"

typedef std::function<void()> TimeOutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeOutCallBack cb;
    bool operator<(const TimerNode& t)
    {
        return expires < t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer() {}
    ~HeapTimer(){}

    void OnAdd(int id, int time_out, const TimeOutCallBack& cb);
    void OnDel(size_t index);
    void OnAdjust(int id, int timeout);
    void OnDoWork(int id);
    void OnTick();          //清除超时节点
    void OnPop();
    void OnClear();
    int GetNextTick();      //下个定时器触发的时间

private:
    void SiftUp(size_t index);
    bool SiftDown(size_t index, size_t n);
    void SwapNode(size_t index, size_t j);

    std::vector<TimerNode> m_timer_heap;
    std::unordered_map<int, size_t> m_ref;
};

#endif //HEAPTIMER_H
