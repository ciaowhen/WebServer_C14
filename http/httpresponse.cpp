//
// Created by user on 2023/1/13.
//

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::m_suffix_type = {
        {".html", "text/html"},
        {".xml", "text/xml"},
        {".xhtml", "text/xhtml"},
        {".txt", "text/plain"},
        {".rtf", "application/rtf"},
        {".pdf", "application/pdf"},
        {".word", "application/nsword"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".gif", "image/gif"},
        {".jpeg", "image/jpeg"},
        {".au", "audio/basic"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"},
        {".css", "text/css"},
        {".js", "text/javascript"},
};

const std::unordered_map<int, std::string> HttpResponse::m_code_status = {
        {200, "OK"},
        {400, "Bad Request"},
        {403, "Forbidden"},
        {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::m_code_path = {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"},
};

HttpResponse::HttpResponse()
{
    m_code = -1;
    m_path = "";
    m_src_dir = "";
    m_is_keep_live = false;
    m_mmfile = nullptr;
    m_mmfile_stat = {0};
}

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Init(const std::string &src_dir, std::string &path, bool is_keep_alive, int code)
{
    assert(src_dir != "");
    if(m_mmfile)
    {
        UnmapFile();
    }

    m_code = code;
    m_is_keep_live = is_keep_alive;
    m_path = path;
    m_src_dir = src_dir;
    m_mmfile = nullptr;
    m_mmfile_stat = {0};
}

void HttpResponse::MakeReponse(Buffer &buff)
{
    if(stat((m_src_dir + m_path).data(), &m_mmfile_stat) < 0 || S_ISDIR(m_mmfile_stat.st_mode))     //判断请求的资源文件
    {
        m_code = 404;
    }
    else if(!(m_mmfile_stat.st_mode & S_IROTH))
    {
        m_code = 404;
    }
    else if(m_code == -1)
    {
        m_code = 200;
    }

    ErrorHtml();
    AddStateLine(buff);
    AddHeader(buff);
    AddContent(buff);
}

char* HttpResponse::File()
{
    return m_mmfile;
}

size_t HttpResponse::GetFileLen() const
{
    return m_mmfile_stat.st_size;
}

void HttpResponse::ErrorHtml()
{
    if(m_code_path.count(m_code) == 1)
    {
        m_path = m_code_path.find(m_code)->second;
        stat((m_src_dir + m_path).data(), &m_mmfile_stat);
    }
}

void HttpResponse::AddStateLine(Buffer &buffer)
{
    std::string status;
    if(m_code_status.count(m_code) == 1)
    {
        status = m_code_status.find(m_code)->second;
    }
    else
    {
        m_code = 400;
        status = m_code_status.find(400)->second;
    }

    buffer.Append("HTTP/1.1 " + std::to_string(m_code) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer &buffer)
{
    buffer.Append("Connection: ");
    if(m_is_keep_live)
    {
        buffer.Append("keep-aliver\r\n");
        buffer.Append("keep-alive:max = 6, timeout = 120\r\n");
    }
    else
    {
        buffer.Append("close\r\n");
    }

    buffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer &buffer)
{
    int src_fd = open((m_src_dir + m_path).data(), O_RDONLY);
    if(src_fd < 0)
    {
        ErrorContent(buffer, "File NotFound!");
        return;
    }

    LOG_DEBUG("file path %s", (m_src_dir + m_path).data());
    int *mm_ret = (int *)mmap(0, m_mmfile_stat.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if(*mm_ret == -1)
    {
        ErrorContent(buffer, "File NotFound!");
        return;
    }

    m_mmfile = (char *)mm_ret;
    close(src_fd);
    buffer.Append("Contet-length: " + std::to_string(m_mmfile_stat.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile()
{
    if(!m_mmfile)
    {
        return;
    }

    munmap(m_mmfile, m_mmfile_stat.st_size);
    m_mmfile = nullptr;
}

std::string HttpResponse::GetFileType()
{
    std::string::size_type idx = m_path.find_last_of('.');
    if(idx == std::string::npos)
    {
        return "text/plain";
    }

    std::string suffix = m_path.substr(idx);
    auto iterator = m_suffix_type.find(suffix);
    if(iterator != m_suffix_type.end())
    {
        return iterator->second;
    }

    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buff, std::string message)
{
    std::string body;
    std::string status;

    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";

    auto iterator = m_code_status.find(m_code);
    if(iterator != m_code_status.end())
    {
        status = iterator->second;
    }
    else
    {
        status = "Bad Request";
    }

    body += std::to_string(m_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
