//
// Created by user on 2023/1/11.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPool
{
public:
    explicit ThreadPool(size_t thread_count = 8):m_pool(std::make_shared<Pool>())
    {
        assert(thread_count > 0);
        for(size_t i = 0; i < thread_count; ++i)
        {
            std::thread([pool = m_pool]
            {
                std::unique_lock<std::mutex> locker(pool->mtx);
                while(true)
                {
                    if(!pool->tasks.empty())
                    {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if (pool->is_close)
                    {
                        break;
                    }
                    else
                    {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();
        }
    }

    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool()
    {
        if(static_cast<bool>(m_pool))
        {
            std::lock_guard<std::mutex> locker(m_pool->mtx);
            m_pool->is_close = true;
        }

        m_pool->cond.notify_all();
    }

    template<class T> void AddTask(T &&task)
    {
        {
            std::lock_guard<std::mutex> locker(m_pool->mtx);
            m_pool->tasks.emplace(std::forward<T>(task));
        }

        m_pool->cond.notify_one();
    }


private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_close;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> m_pool;
};

#endif //THREADPOOL_H
