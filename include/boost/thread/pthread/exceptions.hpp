// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06040709_HPP
#define BOOST_THREAD_RS06040709_HPP

#include <boost/thread/pthread/config.hpp>

#include <string>
#include <stdexcept>

namespace boost {

class BOOST_THREAD_DECL thread_exception : public std::exception
{
protected:
    thread_exception();
    thread_exception(int sys_err_code);

public:
    ~thread_exception() throw();

    int native_error() const;

    const char* message() const;

private:
    int m_sys_err;
};

class BOOST_THREAD_DECL lock_error : public thread_exception
{
public:
    lock_error();
    lock_error(int sys_err_code);
    ~lock_error() throw();

    virtual const char* what() const throw();
};

class BOOST_THREAD_DECL thread_resource_error : public thread_exception
{
public:
    thread_resource_error();
    thread_resource_error(int sys_err_code);
    ~thread_resource_error() throw();

    virtual const char* what() const throw();
};

class BOOST_THREAD_DECL unsupported_thread_option : public thread_exception
{
public:
    unsupported_thread_option();
    unsupported_thread_option(int sys_err_code);
    ~unsupported_thread_option() throw();

    virtual const char* what() const throw();
};

class BOOST_THREAD_DECL invalid_thread_argument : public thread_exception
{
public:
    invalid_thread_argument();
    invalid_thread_argument(int sys_err_code);
    ~invalid_thread_argument() throw();

    virtual const char* what() const throw();
};

class BOOST_THREAD_DECL thread_permission_error : public thread_exception
{
public:
    thread_permission_error();
    thread_permission_error(int sys_err_code);
    ~thread_permission_error() throw();

    virtual const char* what() const throw();
};

} // namespace boost

#endif // BOOST_THREAD_RS06040709_HPP
