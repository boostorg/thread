#ifndef BOOST_THREAD_PTHREAD_MUTEX_HPP
#define BOOST_THREAD_PTHREAD_MUTEX_HPP
// (C) Copyright 2007 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <pthread.h>
#include <boost/utility.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/assert.hpp>
#include <unistd.h>
#include <errno.h>
#include "timespec.hpp"

#ifdef _POSIX_TIMEOUTS
#if _POSIX_TIMEOUTS >= 0
#define BOOST_PTHREAD_HAS_TIMEDLOCK
#endif
#endif

namespace boost
{
    class mutex:
        boost::noncopyable
    {
    private:
        pthread_mutex_t m;
    public:
        mutex()
        {
            int const res=pthread_mutex_init(&m,NULL);
            if(res)
            {
                throw thread_resource_error();
            }
        }
        ~mutex()
        {
            int const res=pthread_mutex_destroy(&m);
            BOOST_ASSERT(!res);
        }
        
        void lock()
        {
            int const res=pthread_mutex_lock(&m);
            BOOST_ASSERT(!res);
        }

        void unlock()
        {
            int const res=pthread_mutex_unlock(&m);
            BOOST_ASSERT(!res);
        }
        
        bool try_lock()
        {
            int const res=pthread_mutex_trylock(&m);
            BOOST_ASSERT(!res || res==EBUSY);
            return !res;
        }
        typedef unique_lock<mutex> scoped_lock;
        typedef scoped_lock scoped_try_lock;
    };

    typedef mutex try_mutex;

    class timed_mutex:
        boost::noncopyable
    {
    private:
        pthread_mutex_t m;
#ifdef BOOST_PTHREAD_HAS_TIMEDLOCK
        pthread_cond_t cond;
        bool is_locked;

        struct pthread_mutex_scoped_lock
        {
            pthread_mutex_t* m;
            explicit pthread_mutex_scoped_lock(pthread_mutex_t* m_):
                m(m_)
            {
                int const res=pthread_mutex_lock(m);
                BOOST_ASSERT(!res);
            }
            ~pthread_mutex_scoped_lock()
            {
                int const res=pthread_mutex_unlock(m);
                BOOST_ASSERT(!res);
            }
            
        };
        
            
#endif
    public:
        timed_mutex()
        {
            int const res=pthread_mutex_init(&m,NULL);
            if(res)
            {
                throw thread_resource_error();
            }
#ifdef BOOST_PTHREAD_HAS_TIMEDLOCK
            int const res2=pthread_cond_init(&cond,NULL);
            if(res2)
            {
                int const destroy_res=pthread_mutex_destroy(&m);
                BOOST_ASSERT(!destroy_res);
                throw thread_resource_error();
            }
            is_locked=false;
#endif
        }
        ~timed_mutex()
        {
            int const res=pthread_mutex_destroy(&m);
            BOOST_ASSERT(!res);
#ifdef BOOST_PTHREAD_HAS_TIMEDLOCK
            int const res2=pthread_cond_destroy(&cond);
            BOOST_ASSERT(!res2);
#endif
        }

        template<typename TimeDuration>
        bool timed_lock(TimeDuration const & relative_time)
        {
            return timed_lock(get_system_time()+relative_time);
        }

#ifdef BOOST_PTHREAD_HAS_TIMEDLOCK
        void lock()
        {
            int const res=pthread_mutex_lock(&m);
            BOOST_ASSERT(!res);
        }

        void unlock()
        {
            int const res=pthread_mutex_unlock(&m);
            BOOST_ASSERT(!res);
        }
        
        bool try_lock()
        {
            int const res=pthread_mutex_trylock(&m);
            BOOST_ASSERT(!res || res==EBUSY);
            return !res;
        }
        bool timed_lock(system_time const & abs_time)
        {
            struct timespec const timeout=detail::get_timespec(abs_time);
            int const res=pthread_mutex_timedlock(&m,&timeout);
            BOOST_ASSERT(!res || res==EBUSY);
            return !res;
        }
#else
        void lock()
        {
            pthread_mutex_scoped_lock const _(&m);
            while(is_locked)
            {
                int const cond_res=pthread_cond_wait(&cond,&m);
                BOOST_ASSERT(!cond_res);
            }
            is_locked=true;
        }

        void unlock()
        {
            pthread_mutex_scoped_lock const _(&m);
            is_locked=false;
            int const res=pthread_cond_signal(&cond);
            BOOST_ASSERT(!res);
        }
        
        bool try_lock()
        {
            pthread_mutex_scoped_lock const _(&m);
            if(is_locked)
            {
                return false;
            }
            is_locked=true;
            return true;
        }

        bool timed_lock(system_time const & abs_time)
        {
            struct timespec const timeout=detail::get_timespec(abs_time);
            pthread_mutex_scoped_lock const _(&m);
            while(is_locked)
            {
                int const cond_res=pthread_cond_timewait(&cond,&m,&timeout);
                if(cond_res==ETIMEOUT)
                {
                    return false;
                }
                BOOST_ASSERT(!cond_res);
            }
            is_locked=true;
            return true;
        }
#endif

        typedef unique_lock<timed_mutex> scoped_timed_lock;
        typedef scoped_timed_lock scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
    };

}


#endif
