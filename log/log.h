//
// Created by user on 2023/1/10.
//

#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include "buffer.h"
#include "blockdeque.h"

class Log
{
public:
    void Init(int level, const char* path = "./log", const char* suffix = ".log", int max_queue_capacity = 1024);
    static Log* Instance();
    static void FlushLogThread();

    void Write(int level, const char *format, ...);
    void Flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() {return m_is_open};

private:
    Log();
    virtual ~Log();
    void AppendLogLevelTitle(int level);
    void AsyncWrite();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* m_path;
    const char* m_suffix;

    int m_max_lines;
    int m_line_count;
    int m_to_day;

    bool m_is_open;

    Buffer m_buff;
    int m_level;
    bool m_is_async;

    FILE *m_file_ptr;
    std::unique_ptr<BlockDeque<std::string>> m_block_deque;
    std::unique_ptr<std::thread> m_write_thread;
    std::mutex m_mutex;
};

#define LOG_BASE(level, format, ...) \
    do{\
        Log *log = Log::Instance();\
        if(log->IsOpen() && log->GetLevel() <= level)\
        {\
          log->Write(level, format, ##_VA_ARGS_);\
          log->flush();\
        }\
    }while(0);

#define LOG_DEBUG(format,...) do {LOG_BASE(0, format, ##_VA_ARGS_)} while(0);
#define LOG_INFO(format,...) do {LOG_BASE(1, format, ##_VA_ARGS_)} while(0);
#define LOG_WARN(format,...) do {LOG_BASE(2, format, ##_VA_ARGS_)} while(0);
#define LOG_ERROR(format,...) do{LOG_BASE(3, format, ##_VA_ARGS_)} while(0);

#endif LOG_H
