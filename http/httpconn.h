//
// Created by user on 2023/1/13.
//

#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <winsock.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void Init(int sockfd, const sockaddr_in& addr);
    void Close();
    ssize_t Read(int *save_errno);
    ssize_t Write(int *save_errno);
    bool Process();

    int GetFd() const {return m_fd;}
    sockaddr_in GetAddr() const {return m_addr;}
    int GetPort() const {return m_addr.sin_port;}
    const char* GetIP() const {return inet_ntoa(m_addr.sin_addr);}
    int ToWriteBytes() {return iov_[0].iov_len + iov[1].iov_len;}
    bool IsKeepAlive() const {return m_request.IsKeepAlive();}

    static bool m_is_et;
    static const char* m_src_dir;
    static std::atomic<int> m_user_count;

private:
    int m_fd;
    struct sockaddr_in m_addr;
    bool m_is_close;
    int m_iov_cnt;
    struct iovec m_iov[2];

    Buffer m_read_buff;
    Buffer m_write_buff;

    HttpRequest m_request;
    HttpResponse m_response;
};

#endif //HTTPCONN_H
