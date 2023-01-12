//
// Created by ciaowhen on 2023/1/5.
//

#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <unistd.h>
#include <vector>
#include <atomic>
#include <string>

#define MAX_BUFFER_SIZE 65535

class Buffer
{
public:
    enum IOV_STATUS
    {
        IS_READ,
        IS_WRITE,
        IS_COUNT
    };

    Buffer(int init_buff_size = 1024);
    ~Buffer() = default;

    size_t GetWritableBytes() const;          //可读
    size_t GetReadableBytes() const;
    size_t GetPrependableBytes() const;

    const char* GetCurReadPosPtr() const;
    void SetEnsureWriteable(size_t len);
    void GetHasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char *end);

    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    void Append(const void *data, size_t len);
    void Append(const Buffer &buff);

    ssize_t ReadFd(int fd, int *error);
    ssize_t WriteFd(int fd, int *error);

private:
    char* GetBeginPtr();
    const char* GetBeginPtr() const;
    void MakeSpace(size_t len);

    std::vector<char> m_buffer;
    std::atomic<std::size_t> m_read_pos;
    std::atomic<std::size_t> m_write_pos;
};

#endif
