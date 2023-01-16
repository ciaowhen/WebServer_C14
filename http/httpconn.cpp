//
// Created by user on 2023/1/13.
//

#include "httpconn.h"

const char* HttpConn::m_src_dir;
std::atomic<int> HttpConn::m_user_count;
bool HttpConn::m_is_et;

HttpConn::HttpConn()
{
    m_fd = -1;
    m_addr = {0};
    m_is_close = true;
}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::Init(int sockfd, const sockaddr_in &addr)
{
    assert(sockfd > 0);
    m_user_count++;
    m_addr = addr;
    m_fd = sockfd;
    m_write_buff.RetrieveAll();
    m_read_buff.RetrieveAll();
    m_is_close = false;
    LOG_INFO("Client[%d](%s:%d) in, UserCount:%d", m_fd, GetIP(), GetPort(), (int)m_user_count);
}

void HttpConn::Close()
{
    m_response.UnmapFile();
    if(m_is_close)
    {
        return;
    }

    m_is_close = true;
    m_user_count--;
    close(m_fd);
    LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", m_fd, GetIP(), GetPort(), (int) m_user_count);
}

ssize_t HttpConn::Read(int *save_errno)
{
    ssize_t len = -1;
    do
    {
        len = m_read_buff.ReadFd(m_fd, save_errno);
        if(len <= 0)
        {
            break;
        }
    } while (m_is_et);

    return len;
}

ssize_t HttpConn::Write(int *save_errno)
{
    ssize_t len = -1;
    do
    {
        len = writev(m_fd, m_iov, m_iov_cnt);
        if(len <= 0)
        {
             *save_errno = errno;
             break;
        }

        if(m_iov[0].iov_len + m_iov[1].iov_len == 0)
        {
            break;
        }
        else if(static_cast<size_t>(len) > m_iov[0].iov_len)
        {
            m_iov[1].iov_base = (uint8_t*) m_iov[1].iov_base + (len - m_iov[0].iov_len);    //重新计算m_iov新的基址及剩余长度
            m_iov[1].iov_len -= (len - m_iov[0].iov_len);
            if(m_iov[0].iov_len)
            {
                m_write_buff.RetrieveAll();
                m_iov[0].iov_len = 0;
            }
        }
        else
        {
            m_iov[0].iov_base = (uint8_t*)m_iov[0].iov_base + len;
            m_iov[0].iov_len -= len;
        }
    } while (m_is_et || ToWriteBytes() > 10240);

    return len;
}

bool HttpConn::Process()
{
    m_request.Init();
    if(m_read_buff.GetReadableBytes() <= 0)
    {
        return false;
    }

    if(m_request.Parse(m_read_buff))
    {
        LOG_DEBUG("%s", m_request.GetPath().c_str());
        m_response.Init(m_src_dir, m_request.GetPath(), m_request.IsKeepAlive(), 200);
    }
    else
    {
        m_response.Init(m_src_dir, m_request.GetPath(), false, 400);
    }

    m_response.MakeReponse(m_write_buff);
    m_iov[0].iov_base = const_cast<char *>(m_write_buff.GetCurReadPosPtr());        //响应头
    m_iov[0].iov_len = m_write_buff.GetReadableBytes();
    m_iov_cnt = 1;

    if(m_response.GetFileLen() > 0 && m_response.File())
    {
        m_iov[1].iov_base = m_response.File();
        m_iov[1].iov_len = m_response.GetFileLen();
        m_iov_cnt = 2;
    }

    LOG_DEBUG("filesize:%d, %d to %d", m_response.GetFileLen(), m_iov_cnt, ToWriteBytes());

    return true;
}