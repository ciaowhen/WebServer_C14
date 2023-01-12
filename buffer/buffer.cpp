//
// Created by ciaowhen on 2023/1/5.
//

#include "buffer.h"
#include <assert.h>
#include <sys/uio.h>

Buffer::Buffer(int init_buff_size)
:m_buffer(init_buff_size)
,m_read_pos(0)
,m_write_pos(0)
{}

size_t Buffer::GetWritableBytes() const
{
    return m_buffer.size() - m_write_pos;
}

size_t Buffer::GetReadableBytes() const
{
    return m_write_pos - m_read_pos;
}

size_t Buffer::GetPrependableBytes() const
{
    return m_read_pos;
}

const char* Buffer::GetCurReadPosPtr() const
{
    return GetBeginPtr() + m_read_pos;
}

void Buffer::SetEnsureWriteable(size_t len)
{
    if(GetWritableBytes() < len)
    {
        MakeSpace(len);
    }

    assert(GetWritableBytes() >= len);
}

void Buffer::GetHasWritten(size_t len)
{
    m_write_pos += len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= GetReadableBytes());
    m_read_pos += len;
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(GetCurReadPosPtr() <= end);
    Retrieve(end - GetCurReadPosPtr());
}

void Buffer::RetrieveAll()
{
    bzero(&m_buffer[0], m_buffer.size());
    m_read_pos = 0;
    m_write_pos = 0;
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(GetCurReadPosPtr(), GetReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst() const
{
    return GetBeginPtr() + m_write_pos;
}

char* Buffer::BeginWrite()
{
    return GetBeginPtr() + m_write_pos;
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.size());
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    SetEnsureWriteable(len);
    std::copy(str, str + len, BeginWrite());
    GetHasWritten(len);
}

void Buffer::Append(const void *data, size_t len)
{
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const Buffer &buff)
{
    Append(buff.GetCurReadPosPtr(), buff.GetWritableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *error)
{
    char buff[MAX_BUFFER_SIZE];
    struct iovec iov[IS_COUNT];
    const size_t writable = GetWritableBytes();

    iov[0].iov_base = GetBeginPtr() + m_write_pos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if(len < 0)
    {
        *error = errno;
    }
    else if(static_cast<size_t>(len) <= writable)
    {
        m_write_pos += len;
    }
    else
    {
        m_write_pos = m_buffer.size();
        Append(buff, len - writable);
    }

    return len;
}

ssize_t Buffer::WriteFd(int fd, int *error)
{
    size_t read_size = GetReadableBytes();
    ssize_t len = write(fd, GetCurReadPosPtr(), read_size);
    if(len < 0)
    {
        *error = errno;
    }

    m_read_pos += len;
    return len;
}

char* Buffer::GetBeginPtr()
{
    return &*m_buffer.begin();
}

const char* Buffer::GetBeginPtr() const
{
    return &*m_buffer.begin();
}

void Buffer::MakeSpace(size_t len)
{
    if(GetWritableBytes() + GetPrependableBytes() < len)
    {
        m_buffer.resize(m_write_pos + len + 1);
    }
    else
    {
        size_t readable = GetReadableBytes();
        std::copy(GetBeginPtr() + m_read_pos, GetBeginPtr() + m_write_pos, GetBeginPtr());
        m_read_pos = 0;
        m_write_pos = m_read_pos + readable;
        assert(readable == GetReadableBytes());
    }
}





