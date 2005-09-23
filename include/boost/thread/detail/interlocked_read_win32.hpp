#ifndef BOOST_THREAD_DETAIL_INTERLOCKED_READ_WIN32_HPP
#define BOOST_THREAD_DETAIL_INTERLOCKED_READ_WIN32_HPP

//  interlocked_read_win32.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>

namespace boost
{
    namespace detail
    {
        inline long interlocked_read(long* x)
        {
            return BOOST_INTERLOCKED_COMPARE_EXCHANGE(x,0,0);
        }
        
        inline void* interlocked_read_pointer(void** x)
        {
            return BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER(x,0,0);
        }
    }
}

#endif
