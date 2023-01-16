//
// Created by user on 2023/1/16.
//

#include "webserver.h"

WebServer::WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger, int sql_port, const char *sql_user,
                     const char *sql_pwd, const char *db_name, int connpool_num, int thread_num, bool open_log,
                     int log_level, int log_queue_size):
                     m_port(port), m_timeout_ms(timeout_ms),m_is_close(false), m_open_linger(opt_linger),
                     m_timer(new HeapTimer()), m_threadpool(new ThreadPool(thread_num)), m_epoller(new Epoller())
{
    m_src_dir = getcwd(nullptr, 256);
    assert(m_src_dir);
    strncat(m_src_dir, "/resources/", 16);
    HttpConn::m_user_count = 0;
    HttpConn::m_src_dir = m_src_dir;
    SqlConnPool::Instance()->Init("loaclhost", sql_port, sql_user, sql_pwd, db_name, connpool_num);

    InitEvenetMode(trig_mode);
    if(!InitSocket())
    {
        m_is_close = true;
    }

    if(open_log)
    {
        Log::Instance()->Init(log_level, "./log", ".log", log_queue_size);
        if(m_is_close)
        {
            LOG_ERROR("================Server init error!================");
        }
        else
        {
            LOG_INFO("================Server init================");
            LOG_INFO("Port:%d, Openlinger: %s", port, opt_linger ? "true" : "false");
            LOG_INFO("Listen mode: %s, OpenConn Mode: %s", (m_listen_event & EPOLLET ? "ET" : "LT"),(m_conn_event & EPOLLET ? "ET" : "LT"));
            LOG_INFO("Log system level: %d", log_level);
            LOG_INFO("Source directory: %s", HttpConn::m_src_dir);
            LOG_INFO("SqlConnPool num:%d, ThreadPool num: %d", connpool_num, thread_num);
        }
    }
}

WebServer::~WebServer()
{
    close(m_listenfd);
    m_is_close = true;
    free(m_src_dir);
    SqlConnPool::Instance()->ClosePool();
}

