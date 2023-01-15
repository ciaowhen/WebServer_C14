//
// Created by user on 2023/1/12.
//

#include "sqlconnpool.h"

SqlConnPool::SqlConnPool()
{
    m_max_conn_count = 0;
    m_use_count = 0;
    m_free_count = 0;
}

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool connPool;
    return &connPool;
}


void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *db_name,
                       int conn_size)
{
    assert(conn_size > 0);
    for(int i = 0; i < conn_size; ++i)
    {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if(!sql)
        {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }

        sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
        if(!sql)
        {
            LOG_ERROR("MySql connect error!");
        }

        m_conn_queue.push(sql);
    }
    m_max_conn_count = conn_size;
    sem_init(&m_sem_id, 0, m_max_conn_count);
}

MYSQL *SqlConnPool::GetConn()
{
    MYSQL *sql = nullptr;
    if(m_conn_queue.empty())
    {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }

    sem_wait(&m_sem_id);

    {
        std::lock_guard<std::mutex> locker(m_mutex);
        sql = m_conn_queue.front();
        m_conn_queue.pop();
    }

    return sql;
}

void SqlConnPool::FreeConn(MYSQL *conn)
{
    assert(conn);
    std::lock_guard<std::mutex> locker(m_mutex);
    m_conn_queue.push(conn);
    sem_post(&m_sem_id);
}

void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    while(!m_conn_queue.empty())
    {
        auto item = m_conn_queue.front();
        m_conn_queue.pop();
        mysql_close(item);
    }

    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_conn_queue.size();
}



