//
// Created by user on 2023/1/13.
//

#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::m_default_html{"/index", "/register", "/login", "/welcome", "/video", "/picture",};
const std::unordered_map<std::string, int> HttpRequest::m_default_html_tag{{"/register.html", 0}, {"/login.html", 1}};

void HttpRequest::Init()
{
    m_method = "";
    m_path = "";
    m_version = "";
    m_body = "";
    m_state = REQUEST_LINE;
    m_header.clear();
    m_post.clear();
}

bool HttpRequest::IsKeepAlive() const
{
    if(m_header.count("Connection") == 1)
    {
        return m_header.find("Connection")->second == "keep-alive" && m_version == "1.1";
    }

    return false;
}

bool HttpRequest::Parse(Buffer &buffer)
{
    const char crlf[] = "\r\n";
    if(buffer.GetReadableBytes() <= 0)
    {
        return  false;
    }

    while (buffer.GetReadableBytes() && m_state != FINISH)
    {
        const char *line_end = std::search(buffer.GetCurReadPosPtr(), buffer.BeginWriteConst(), crlf, crlf + 2);
        std::string line(buffer.GetCurReadPosPtr(), line_end);
        switch(m_state)
        {
            case REQUEST_LINE:
            {
                if(!ParseRequestLine(line))
                {
                    return false;
                }
                ParsePath();
            }
            break;
            case HEADERS:
            {
                ParseHeader(line);
                if(buffer.GetReadableBytes() <= 2)
                {
                    m_state = FINISH;
                }
            }
            break;
            case BODY:
            {
                ParseBody(line);
            }
            break;
            default:
            break;
        }
        if(line_end == buffer.BeginWriteConst())
        {
            break;
        }

        buffer.RetrieveUntil(line_end + 2);
    }

    LOG_ERROR("[%s],[%s],[%s]", m_method.c_str(), m_path.c_str(), m_version.c_str());
    return true;
}

void HttpRequest::ParsePath()
{
    if(m_path == "/")
    {
        m_path = "/index.html";
    }
    else
    {
        for(auto &item: m_default_html)
        {
            if(item == m_path)
            {
                m_path += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine(const std::string &line)
{
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;
    if(std::regex_match(line,sub_match,patten))
    {
        m_method = sub_match[1];
        m_path = sub_match[2];
        m_version = sub_match[3];
        m_state = HEADERS;
        return true;
    }

    LOG_ERROR("RequestLine error");
    return false;
}

void HttpRequest::ParseHeader(const std::string &line)
{
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if(std::regex_match(line, sub_match, pattern))
    {
        m_header[sub_match[1]] = sub_match[2];
    }
    else
    {
        m_state = BODY;
    }
}

void HttpRequest::ParseBody(const std::string &line)
{
    m_body = line;
    ParsePost();
    m_state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::ConverHex(char ch)
{
    if(ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }

    if(ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }

    return ch;
}

void HttpRequest::ParsePost()
{
    if(m_method != "POST" || m_header["Content-Type"] != "application/x-www-form-urlencoded")
    {
        return;
    }

    ParseFormUrlEncoded();
    if (!m_default_html_tag.count(m_path))
    {
        return;
    }

    int tag = m_default_html_tag.find(m_path)->second;
    LOG_DEBUG("Tag:%d", tag);
    if (tag != 0 && tag != 1)
    {
        return;
    }

    bool is_login = (tag == 1);
    if (UserVerify(m_post["username"], m_post["password"], is_login))
    {
        m_path = "/welcome.html";
    }
    else
    {
        m_path = "/error.html";
    }

    return;
}

void HttpRequest::ParseFormUrlEncoded()
{
    if(m_body.size() == 0)
    {
        return;
    }

    std::string key, value;
    int num = 0;
    int body_size = m_body.size();

    int i = 0, j = 0;

    for(; i < body_size; ++i)
    {
        char ch = m_body[i];
        switch (ch)
        {
            case '=':
                key = m_body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                m_body[i] = ' ';
                break;
            case '%':
                num = ConverHex(m_body[i + 1]) * 16 + ConverHex(m_body[i + 2]);
                m_body[i + 2] = num % 10 + '0';
                m_body[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = m_body.substr(j, i - j);
                j = i + 1;
                m_post[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }

    assert(j <= i);
    if(m_post.count(key) == 0 && j < i)
    {
        value = m_body.substr(j, i - j);
        m_post[key] = value;
    }
}