//
// Created by user on 2023/1/13.
//

#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

class SqlConnRAII
{
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *conn_pool)
    {
        assert(conn_pool);
        *sql = conn_pool->GetConn();
        m_sql = *sql;
        m_conn_pool = conn_pool;
    }

    ~SqlConnRAII()
    {
        if(m_sql)
        {
            m_conn_pool->FreeConn();
        }
    }

private:
    MYSQL *m_sql;
    SqlConnPool *m_conn_pool;
};

#endif //SQLCONNRAII_H
