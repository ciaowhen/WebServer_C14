//
// Created by user on 2023/1/16.
//

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socker.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/heaptimer.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnRAII.h"
#include "../http/httpconn.h"

class WebServer
{
public:
    WebServer(int port, int trig_mode, int timeout_ms, bool opt_linger, int sql_port, const char *sql_user, const char *sql_pwd,
              const char *db_name, int connpool_num, int thread_num, bool open_log, int log_level, int log_queue_size);
    ~WebServer();
    void Start();

private:
    bool InitSocket();
    void InitEvenetMode(int trig_mode);
    void AddClient(int fd, sockaddr_in addr);

    void DealListen();
    void DealWrite(HttpConn *client);
    void DealRead(HttpConn *client);

    void SendError(int fd, const char *info);
    void ExtentTime(HttpConn *client);
    void CloseConn(HttpConn *client);

    void OnRead(HttpConn *client);
    void OnWrite(HttpConn *client);
    void OnProcess(HttpConn *client);

    static const int MAX_FD = 65536;
    static int SetFdNonBlock(int fd);

    int m_port;
    int m_timeout_ms;
    int m_listenfd;
    bool m_is_close;
    bool m_open_linger;
    char *m_src_dir;

    uint32_t m_listen_event;
    uint32_t m_conn_event;

    std::unique_ptr<HeapTimer> m_timer;
    std::unique_ptr<ThreadPool> m_threadpool;
    std::unique_ptr<Epoller> m_epoller;
    std::unordered_map<int, HttpConn> m_users;
};

#endif //WEBSERVER_H
