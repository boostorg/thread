#ifndef BOOST_THREAD_DETAIL_INTERLOCKED_READ_WIN32_HPP
#define BOOST_THREAD_DETAIL_INTERLOCKED_READ_WIN32_HPP

//  interlocked_read_win32.hpp
//
//  (C) Copyright 2005-7 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>

namespace boost
{
    namespace detail
    {
        inline long interlocked_read_acquire(long volatile* x)
        {
            long const res=*x;
            _ReadWriteBarrier();
            return res;
        }
        inline void* interlocked_read_acquire(void* volatile* x)
        {
            void* const res=*x;
            _ReadWriteBarrier();
            return res;
        }
    }
}

#endif
