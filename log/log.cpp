//
// Created by user on 2023/1/10.
//

#include "log.h"

Log::Log()
{
    m_max_lines = 0;
    m_line_count = 0;
    m_to_day = 0;
    m_is_open = false;
    m_level = 0;
    m_is_async = false;
    m_write_thread = nullptr;
    m_block_deque = nullptr;
    m_file_ptr = nullptr;
}

Log::~Log()
{
    if()
}