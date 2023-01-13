//
// Created by user on 2023/1/13.
//

#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::m_default_html{"/index", "/register", "/login", "/welcome", "/video", "/picture",};
const std::unordered_map<std::string, int> HttpRequest::m_default_html_tag{{"/register.html", 0}, {"/login.html", 1}};
