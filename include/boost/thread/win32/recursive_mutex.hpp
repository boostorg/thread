#ifndef BOOST_RECURSIVE_MUTEX_WIN32_HPP
#define BOOST_RECURSIVE_MUTEX_WIN32_HPP

//  recursive_mutex.hpp
//
//  (C) Copyright 2006-7 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <boost/utility.hpp>
#include "basic_recursive_mutex.hpp"
#include <boost/thread/exceptions.hpp>
#include <boost/thread/locks.hpp>

namespace boost
{
    class recursive_mutex:
        boost::noncopyable,
        public ::boost::detail::basic_recursive_mutex
    {
    public:
        recursive_mutex()
        {
            ::boost::detail::basic_recursive_mutex::initialize();
        }
        ~recursive_mutex()
        {
            ::boost::detail::basic_recursive_mutex::destroy();
        }

        typedef unique_lock<recursive_mutex> scoped_lock;
        typedef scoped_lock scoped_try_lock;
    };

    typedef recursive_mutex recursive_try_mutex;

    class recursive_timed_mutex:
        boost::noncopyable,
        public ::boost::detail::basic_recursive_timed_mutex
    {
    public:
        recursive_timed_mutex()
        {
            ::boost::detail::basic_recursive_timed_mutex::initialize();
        }
        ~recursive_timed_mutex()
        {
            ::boost::detail::basic_recursive_timed_mutex::destroy();
        }

        typedef unique_lock<recursive_timed_mutex> scoped_timed_lock;
        typedef scoped_timed_lock scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
    };
}


#endif
