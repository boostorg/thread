#ifndef BOOST_THREAD_ONCE_HPP
#define BOOST_THREAD_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2006-7 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/platform.hpp>
#include BOOST_THREAD_PLATFORM(once.hpp)

namespace boost
{
    inline void call_once(void (*func)(),once_flag& flag)
    {
        call_once(flag,func);
    }
}

#endif
