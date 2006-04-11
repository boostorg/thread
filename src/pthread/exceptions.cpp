// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>
#include <boost/thread/pthread/exceptions.hpp>
#include <cstring>
#include <string>

# ifdef BOOST_NO_STDC_NAMESPACE
namespace std { using ::strerror; }
# endif

#include <errno.h> // for POSIX error codes

namespace
{

std::string system_message(int sys_err_code)
{
    std::string str;
    str += std::strerror(errno);
    return str;
}

} // unnamed namespace

namespace boost {

thread_exception::thread_exception()
    : m_sys_err(0)
{
}

thread_exception::thread_exception(int sys_err_code)
    : m_sys_err(sys_err_code)
{
}

thread_exception::~thread_exception() throw()
{
}

int thread_exception::native_error() const
{
    return m_sys_err; 
}

const char* thread_exception::message() const
{
    if (m_sys_err != 0)
        return system_message(m_sys_err).c_str();
    return what();
}

lock_error::lock_error()
{
}

lock_error::lock_error(int sys_err_code)
    : thread_exception(sys_err_code)
{
}

lock_error::~lock_error() throw()
{
}

const char* lock_error::what() const throw()
{
    return "boost::lock_error";
}

thread_resource_error::thread_resource_error()
{
}

thread_resource_error::thread_resource_error(int sys_err_code)
    : thread_exception(sys_err_code)
{
}

thread_resource_error::~thread_resource_error() throw()
{
}

const char* thread_resource_error::what() const throw()
{
    return "boost::thread_resource_error";
}

unsupported_thread_option::unsupported_thread_option()
{
}

unsupported_thread_option::unsupported_thread_option(int sys_err_code)
    : thread_exception(sys_err_code)
{
}

unsupported_thread_option::~unsupported_thread_option() throw()
{
}

const char* unsupported_thread_option::what() const throw()
{
    return "boost::unsupported_thread_option";
}

invalid_thread_argument::invalid_thread_argument()
{
}

invalid_thread_argument::invalid_thread_argument(int sys_err_code)
    : thread_exception(sys_err_code)
{
}

invalid_thread_argument::~invalid_thread_argument() throw()
{
}

const char* invalid_thread_argument::what() const throw()
{
    return "boost::invalid_thread_argument";
}

thread_permission_error::thread_permission_error()
{
}

thread_permission_error::thread_permission_error(int sys_err_code)
    : thread_exception(sys_err_code)
{
}

thread_permission_error::~thread_permission_error() throw()
{
}

const char* thread_permission_error::what() const throw()
{
    return "boost::thread_permission_error";
}

} // namespace boost
