//
// Created by user on 2023/1/12.
//

#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool
{
public:
    void Init(const char *host, int port, const char *user, const char *pwd, const char *db_name, int conn_size = 10);
    void ClosePool();

    static SqlConnPool *Instance();
    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();


private:
    SqlConnPool();
    ~SqlConnPool();

    int m_max_conn_count;
    int m_use_count;
    int m_free_count;

    std::queue<MYSQL *> m_conn_queue;
    std::mutex m_mutex;
    sem_t m_sem_id;
};

#endif //SQLCONNPOOL_H
