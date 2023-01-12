//
// Created by user on 2023/1/10.
//

#include "log.h"
#include <sys/stat.h>
#include <stdarg.h>
#include <iostream>
#include <assert.h>

Log::Log()
{
    m_max_lines = 0;
    m_line_count = 0;
    m_to_day = 0;
    m_is_open = false;
    m_level = 0;
    m_is_async = false;
    m_write_thread = nullptr;
    m_block_deque = nullptr;
    m_file_ptr = nullptr;
}

Log::~Log()
{
    if(m_write_thread && m_write_thread->joinable())
    {
        while(!m_block_deque->Empty())
        {
            m_block_deque->Flush();
        }

        m_block_deque->Close();
        m_write_thread->join();
    }

    if(m_file_ptr)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        Flush();
        fclose(m_file_ptr);
    }
}

int Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_level;
}

void Log::SetLevel(int level)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_level = level;
}

void Log::Init(int level, const char *path, const char *suffix, int max_queue_capacity)
{
    if(max_queue_capacity > 0)
    {
        m_is_async = true;
        if(!m_block_deque)
        {
            std::unique_ptr<BlockQueue<std::string>> new_deque(new BlockQueue<std::string>);
            m_block_deque = move(new_deque);

            std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));
            m_write_thread = move(new_thread);
        }
    }
    else
    {
        m_is_async = false;
    }

    m_line_count = 0;
    time_t timer = time(nullptr);
    struct tm *sys_time = localtime(&timer);
    struct tm t = *sys_time;
    m_path = path;
    m_suffix = suffix;
    char filename[LOG_NAME_LEN] = {0};
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", m_path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, m_suffix);
    m_to_day = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_buff.RetrieveAll();
        if(m_file_ptr)
        {
            Flush();
            fclose(m_file_ptr);
        }

        m_file_ptr = fopen(filename, "a");
        if(m_file_ptr == nullptr)
        {
            mkdir(m_path);
            chmod(m_path, 0777);
            m_file_ptr = fopen(filename, "a");
        }

        assert(m_file_ptr != nullptr);
    }

    m_level = level;
    m_is_open = true;
}

void Log::Write(int level, const char *format, ...)
{
    struct timeval now = {0,0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm *sys_time = localtime(&t_sec);
    struct tm t = *sys_time;
    va_list v_list;

    if(m_to_day != t.tm_mday || (m_line_count && (m_line_count % m_max_lines == 0)))
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        locker.unlock();

        char new_file[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if(m_to_day != t.tm_mday)
        {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", m_path, tail, m_suffix);
            m_to_day = t.tm_mday;
            m_line_count = 0;
        }
        else
        {
            snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", m_path, tail, (m_line_count / MAX_LINES), m_suffix);
        }

        locker.lock();
        Flush();
        fclose(m_file_ptr);
        m_file_ptr = fopen(new_file, "a");
        assert(m_file_ptr != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_line_count++;
        int n = snprintf(m_buff.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        m_buff.GetHasWritten(n);

        AppendLogLevelTitle(level);
        va_start(v_list,format);
        int m = vsnprintf(m_buff.BeginWrite(), m_buff.GetWritableBytes(), format, v_list);
        va_end(v_list);

        if(m_is_async && m_block_deque && !m_block_deque->Full())
        {
            m_block_deque->PushBack(m_buff.RetrieveAllToStr());
        }
        else
        {
            fputs(m_buff.GetCurReadPosPtr(), m_file_ptr);
        }

        m_buff.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle(int level)
{
    switch (level)
    {
        case LL_DEBUG:
            m_buff.Append("[Debug]: ", 9);
            break;
        case LL_INFO:
            m_buff.Append("[Info]: ", 9);
            break;
        case LL_WARN:
            m_buff.Append("[Warn]: ", 9);
            break;
        case LL_ERROR:
            m_buff.Append("[Error]: ", 9);
            break;
        default:
            m_buff.Append("[Info]: ", 9);
            break;
    }
}

void Log::Flush()
{
    if(m_is_async)
    {
        m_block_deque->Flush();
    }

    fflush(m_file_ptr);
}

void Log::AsyncWrite()
{
    std::string str = "";
    while (m_block_deque->Pop(str))
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        fputs(str.c_str(), m_file_ptr);
    }
}

Log *Log::Instance()
{
    static Log inst;
    return &inst;
}

void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite();
}
