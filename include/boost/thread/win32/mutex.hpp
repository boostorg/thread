#ifndef BOOST_THREAD_WIN32_MUTEX_HPP
#define BOOST_THREAD_WIN32_MUTEX_HPP
// (C) Copyright 2005-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "basic_timed_mutex.hpp"
#include <boost/utility.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/locks.hpp>

namespace boost
{
    namespace detail
    {
        typedef ::boost::detail::basic_timed_mutex underlying_mutex;
    }

    class mutex:
		boost::noncopyable,
        public ::boost::detail::underlying_mutex
    {
    public:
        mutex()
        {
            initialize();
        }
        ~mutex()
        {
            destroy();
        }

        typedef unique_lock<mutex> scoped_lock;
        typedef scoped_lock scoped_try_lock;
    };

    typedef mutex try_mutex;

    class timed_mutex:
        boost::noncopyable,
        public ::boost::detail::basic_timed_mutex
    {
    public:
        timed_mutex()
        {
            initialize();
        }

        ~timed_mutex()
        {
            destroy();
        }

        typedef unique_lock<timed_mutex> scoped_timed_lock;
        typedef scoped_timed_lock scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
    };
}

#endif
