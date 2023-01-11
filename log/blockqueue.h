//
// Created by ciaowhen on 2022/12/24.
//

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <cassert>

template<typename T>
class BlockQueue
{
public:
    explicit BlockQueue(size_t max_capacity = 1000);       //禁止隐式类型转换
    ~BlockQueue();

    void PushBack(const T &item);
    void PushFront(const T &item);
    bool Pop(const T &item);
    bool Pop(const T &item, int timeout);
    bool Empty();
    bool Full();
    size_t Size();
    size_t Capacity();
    void Clear();
    void Close();
    void Flush();
    T Front();
    T Back();

private:
    std::deque<T> m_block_queue;
    size_t m_capacity;
    bool m_is_close;
    std::mutex m_mutex;
    std::condition_variable m_producer;         //生产者
    std::condition_variable m_consumer;         //消费者
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t max_capacity)
:m_capacity(max_capacity),
 m_is_close(false)
{
    assert(max_capacity > 0);
}

template<typename T>
BlockQueue<T>::~BlockQueue()
{
    Close();
}

template<typename T>
void BlockQueue<T>::PushBack(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    m_producer.wait(locker, [=](){
        return m_block_queue.size() < m_capacity;
    });

    m_block_queue.push_back(item);
    m_consumer.notify_one();
}

template<typename T>
void BlockQueue<T>::PushFront(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    m_producer.wait(locker, [=](){
        return m_block_queue.size() < m_capacity;
    });

    m_block_queue.push_front(item);
    m_consumer.notify_one()
}

template<typename T>
bool BlockQueue<T>::Pop(const T &item)
{
    std::unique_lock<std::mutex> locker(m_mutex);
    m_consumer.wait(locker,[=](){
        return !m_block_queue.empty();
    })

    if(m_is_close)
    {
        return false;
    }

    item = m_block_queue.front();
    m_block_queue.pop_front();
    m_producer.notify_one();

    return true;
}

template<typename T>
bool BlockQueue<T>::Pop(const T &item, int timeout)
{
    std::unique_lock<std::mutex> locker(m_mutex);

    if(m_consumer.wait_for(locker, std::chrono::seconds(timeout), [=](){
        return !m_block_queue.empty();
    }) == std::cv_status::timeout)
    {
        return false;
    }

    if(m_is_close)
    {
        return false;
    }

    item = m_block_queue.front();
    m_block_queue.pop_front();
    m_producer.notify_one();

    return true;
}

template<typename T>
bool BlockQueue<T>::Empty()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.empty();
}

template<typename T>
bool BlockQueue<T>::Full()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.size() >= m_capacity;
}

template<typename T>
size_t BlockQueue<T>::Size()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.size();
}

template<typename T>
size_t BlockQueue<T>::Capacity()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_capacity;
}

template<typename T>
void BlockQueue<T>::Clear()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_block_queue.clear();
}

template<typename T>
void BlockQueue<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_block_queue.clear();
        m_is_close = true;
    }

    m_consumer.notify_all();
    m_producer.notify_all();
}

template<typename T>
void BlockQueue<T>::Flush()
{
    m_consumer.notify_one();
}

template<typename T>
T BlockQueue<T>::Front()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.front();
}

template<typename T>
T BlockQueue<T>::Back()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_block_queue.back();
}

#endif
