// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/detail/config.hpp>

#include <boost/thread/detail/named.hpp>
#include <string.h>
#include <malloc.h>

namespace {

char* concat(char* result, const char* buf)
{
    if (result == 0)
        return strdup(buf);
    
    int len = strlen(result) + strlen(buf) + 1;
    result = (char*)realloc(result, len);
    strcat(result, buf);
    return result;
}

char* get_root()
{
#if defined(BOOST_HAS_WINTHREADS)
    return "";
#elif defined(BOOST_HAS_PTHREADS)
    return "/";
#else
    return "";
#endif
}

char* encode(const char* str)
{
    const char* digits="0123456789abcdef";
    char* result=0;
    static char buf[100];
    char* ebuf = buf + 100;
    char* p = buf;
    *p = 0;
    strcat(p, get_root());
    p = p + strlen(p);
    while (*str)
    {
        if (((*str >= '0') && (*str <= '9')) ||
            ((*str >= 'a') && (*str <= 'z')) ||
            ((*str >= 'A') && (*str <= 'Z')) ||
            (*str == '/') || (*str == '.') || (*str == '_'))
        {
            *p = *str;
        }
        else if (*str == ' ')
        {
            *p = '+';
        }
        else
        {
            if (p + 3 >= ebuf)
            {
                *p = 0;
                result = concat(result, buf);
                p = buf;
            }
            *p = '%';
            char* e = p + 2;
            int v = *str;
            while (e > p)
            {
                *e-- = digits[v % 16];
                v /= 16;
            }
            p += 2;
        }
        if (++p == ebuf)
        {
            *p = 0;
            result = concat(result, buf);
            p = buf;
        }
        ++str;
    }
    *p = 0;
    result = concat(result, buf);
    return result;
}

} // namespace

namespace boost {
namespace detail {

named_object::named_object(const char* name)
    : m_name(0), m_ename(0)
{
    if (name)
    {
        m_name = strdup(name);
        if (*m_name == '%')
            m_ename = m_name + 1;
        else
            m_ename = encode(name);
    }
}

named_object::~named_object()
{
    if (m_name)
    {
        if (*m_name != '%')
            free(m_ename);
        free(m_name);
    }
}

const char* named_object::name() const
{
    return m_name;
}

const char* named_object::effective_name() const
{
    return m_ename;
}

} // namespace detail
} // namespace boost
