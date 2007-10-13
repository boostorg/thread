#ifndef BOOST_THREAD_PTHREAD_ONCE_HPP
#define BOOST_THREAD_PTHREAD_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2007 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <pthread.h>
#include <boost/assert.hpp>

namespace boost {

    struct once_flag
    {
        pthread_mutex_t mutex;
        unsigned flag;
    };

#define BOOST_ONCE_INIT {PTHREAD_MUTEX_INITIALIZER,0}

    namespace detail
    {
        struct pthread_mutex_scoped_lock
        {
            pthread_mutex_t * mutex;
            
            explicit pthread_mutex_scoped_lock(pthread_mutex_t* mutex_):
                mutex(mutex_)
            {
                int const res=pthread_mutex_lock(mutex);
                BOOST_ASSERT(!res);
            }
            ~pthread_mutex_scoped_lock()
            {
                int const res=pthread_mutex_unlock(mutex);
                BOOST_ASSERT(!res);
            }
        };
    }

    template<typename Function>
    void call_once(once_flag& flag,Function f)
    {
        long const function_complete_flag_value=0xc15730e2;

#ifdef BOOST_PTHREAD_HAS_ATOMICS
        if(::boost::detail::interlocked_read_acquire(&flag.flag)!=function_complete_flag_value)
        {
#endif
            detail::pthread_mutex_scoped_lock const lock(&flag.mutex);
            if(flag.flag!=function_complete_flag_value)
            {
                f();
#ifdef BOOST_PTHREAD_HAS_ATOMICS
                ::boost::detail::interlocked_write_release(&flag.flag,function_complete_flag_value);
#else
                flag.flag=function_complete_flag_value;
#endif
            }
#ifdef BOOST_PTHREAD_HAS_ATOMICS
        }
#endif
    }
    

}

#endif
