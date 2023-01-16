//
// Created by user on 2023/1/13.
//

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& src_dir, std::string& path, bool is_keep_alive = false, int code = -1);
    void MakeReponse(Buffer& buff);
    void UnmapFile();
    char* File();
    size_t GetFileLen() const;
    void ErrorContent(Buffer& buff, std::string message);
    int GetCode() const { return m_code;}

private:
    void AddStateLine(Buffer &buffer);
    void AddHeader(Buffer &buffer);
    void AddContent(Buffer &buffer);

    void ErrorHtml();
    std::string GetFileType();

    std::string m_path;
    std::string m_src_dir;
    int m_code;
    bool m_is_keep_live;
    char* m_mmfile;
    struct stat m_mmfile_stat;
    static const std::unordered_map<std::string, std::string> m_suffix_type;
    static const std::unordered_map<int, std::string> m_code_status;
    static const std::unordered_map<int, std::string> m_code_path;
};

#endif //HTTPRESPONSE_H
