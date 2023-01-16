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

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool is_login)
{
    if(name == "" || pwd == "")
    {
        return false;
    }

    LOG_INFO("Verify name:%s pwd%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    char order[256] = {0};

    if(!is_login)
    {
        flag = true;
    }

    snprintf(order, 256, "SELECT uername, password FROM user WHERE username = ‘%s’ LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    MYSQL_RES *mysql_res = nullptr;
    if(mysql_query(sql, order))
    {
        mysql_free_result(mysql_res);
        mysql_res = nullptr;
        return false;
    }

    mysql_res = mysql_store_result(sql);
    unsigned int result_num = mysql_num_fields(mysql_res);
    MYSQL_FIELD *mysql_fileds = mysql_fetch_field(mysql_res);

    while (MYSQL_ROW mysql_row = mysql_fetch_row(mysql_res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", mysql_row[0], mysql_row[1]);
        std::string password(mysql_row[1]);

        if(is_login)                //登录行为
        {
            if(password == pwd)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("password is error!");
            }
        }
        else                        //注册行为
        {
            flag = false;
            LOG_DEBUG("user used!");
        }
    }

    mysql_free_result(mysql_res);

    if(!is_login && flag)           //注册行为
    {
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }

        flag = true;
    }

    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify success!");
    return flag;
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    auto iterator = m_post.find(key);
    if(iterator != m_post.end())
    {
        return iterator->second;
    }

    return "";
}

std::string HttpRequest::GetPost(const std::string &key) const
{
    assert(key != "");
    auto iterator = m_post.find(key);
    if(iterator != m_post.end())
    {
        return iterator->second;
    }

    return "";
}
