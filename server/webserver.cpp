//
// Created by user on 2023/1/16.
//

#include <ws2tcpip.h>
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
            LOG_ERROR("================ Server Init Error! ================");
        }
        else
        {
            LOG_INFO("================ Server Init ================");
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

void WebServer::Start()
{
    int time_ms = -1;
    if(!m_is_close)
    {
        LOG_INFO("================ Server Start ================");
    }

    while(!m_is_close)
    {
        if(m_timeout_ms > 0)
        {
            time_ms = m_timer->GetNextTick();
        }

        int event_cnt = m_epoller->Wait(time_ms);
        for(int i = 0; i < event_cnt; ++i)
        {
            int fd = m_epoller->GetEventFd(i);
            uint32_t  events = m_epoller->GetEvents(i);
            if(fd == m_listenfd)
            {
                DealListen();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(m_users.count(fd) > 0);
                CloseConn(&m_users[fd]);
            }
            else if(events & EPOLLIN)
            {
                assert(m_users.count(fd) > 0);
                DealRead(&m_users[fd]);
            }
            else if(events & EPOLLOUT)
            {
                assert(m_users.count(fd) > 0);
                DealWrite(&m_users[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

bool WebServer::InitSocket()
{
    int ret = 0;
    struct sockaddr_in addr;
    if(m_port > 65535 || m_port < 1024)
    {
        LOG_ERROR("Port:%d error!", m_port);
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    struct linger opt_linger = {0};
    if(m_open_linger)
    {
        //优雅关闭：直到所剩数据发送完毕或超时
        opt_linger.l_onoff = 1;
        opt_linger.l_linger = 1;
    }

    m_listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_listenfd < 0)
    {
        LOG_ERROR("Create socket error!");
        return false;
    }

    ret = setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, (const char*)&opt_linger, sizeof(opt_linger));
    if(ret < 0)
    {
        LOG_ERROR("Set socket setsockopt error!");
        close(m_listenfd);
        return false;
    }

    ret = bind(m_listenfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0)
    {
        LOG_ERROR("Bind port:%d error!", m_port);
        close(m_listenfd);
        return false;
    }

    ret = listen(m_listenfd, 6);
    if(ret < 0)
    {
        LOG_ERROR("Listen port:%d error!", m_port);
        close(m_listenfd);
        return false;
    }

    ret = m_epoller->AddFd(m_listenfd, m_listen_event | EPOLLIN);
    if(ret == 0)
    {
        LOG_ERROR("Add listen fd error!");
        close(m_listenfd);
        return false;
    }

    SetFdNonBlock(m_listenfd);
    LOG_INFO("Server port:%d success!", m_port);
    return true;
}

void WebServer::InitEvenetMode(int trig_mode)
{
    m_listen_event = EPOLLRDHUP;
    m_conn_event = EPOLLONESHOT | EPOLLRDHUP;
    switch(trig_mode)
    {
        case 0:
            break;
        case 1:
            m_conn_event |= EPOLLET;
            break;
        case 2:
            m_listen_event |= EPOLLET;
            break;
        case 3:
            m_listen_event |= EPOLLET;
            m_conn_event |= EPOLLET;
            break;
        default:
            m_listen_event |= EPOLLET;
            m_conn_event |= EPOLLET;
            break;
    }

    HttpConn::m_is_et = (m_conn_event & EPOLLET);
}

void WebServer::AddClient(int fd, sockaddr_in addr)
{
    assert(fd > 0);
    m_users[fd].Init(fd, addr);
    if(m_timeout_ms > 0)
    {
        m_timer->OnAdd(fd, m_timeout_ms, std::bind(&WebServer::CloseConn, this, &m_users[fd]));
    }

    m_epoller->AddFd(fd, EPOLLIN | m_conn_event);
    SetFdNonBlock(fd);
    LOG_INFO("Client[%d] in!", m_users[fd].GetFd());
}

void WebServer::DealListen()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do
    {
        int fd = accept(m_listenfd, (struct sockaddr *)&addr, &len);
        if(fd <= 0)
        {
            return;
        }
        else if(HttpConn::m_user_count >= MAX_FD)
        {
            SendError(fd, "Sevrer busy!");
            LOG_WARN("Clinets is full!");
            return;
        }

        AddClient(fd, addr);
    } while (m_listen_event & EPOLLET);
}

void WebServer::DealWrite(HttpConn *client)
{
    assert(client);
    ExtentTime(client);
    m_threadpool->AddTask(std::bind(&Webserver::OnWrite, this, client));
}

void WebServer::DealRead(HttpConn *client)
{
    assert(client);
    ExtentTime(client);
    m_threadpool->AddTask(std::bind(&WebServer::OnRead, this, client));
}

void WebServer::SendError(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0)
    {
        LOG_WARN("send error to client[%d] error!", fd);
    }

    close(fd);
}

void WebServer::ExtentTime(HttpConn *client)
{
    assert(client);
    if(m_timeout_ms > 0)
    {
        m_timer->OnAdjust(client->GetFd(), m_timeout_ms);
    }
}

void WebServer::CloseConn(HttpConn *client)
{
    assert(client);
    LOG_INFO("Client[%d] close connection!", client->GetFd());
    m_epoller->DelFd(client->GetFd());
    client->Close();
}

void WebServer::OnRead(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int read_error = 0;
    ret = client->Read(&read_error);
    if(ret <= 0 && read_error != EAGAIN)
    {
        CloseConn(client);
        return;
    }

    OnProcess(client);
}

void WebServer::OnWrite(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int write_error = 0;
    ret = client->Write(&write_error);
    if(client->ToWriteBytes() == 0)
    {
        if(client->IsKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if(write_error == EAGAIN)
        {
            m_epoller->ModFd(client->GetFd(),m_conn_event | EPOLLOUT);
            return;
        }
    }

    CloseConn(client);
}

void WebServer::OnProcess(HttpConn *client)
{
    assert(client);
    if(client->Process())
    {
        m_epoller->ModFd(client->GetFd(), m_conn_event | EPOLLOUT);
    }
    else
    {
        m_epoller->ModFd(client->GetFd(), m_conn_event | EPOLLIN);
    }
}

int WebServer::SetFdNonBlock(int fd)
{
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

