#ifndef BOOST_THREAD_PTHREAD_RECURSIVE_MUTEX_HPP
#define BOOST_THREAD_PTHREAD_RECURSIVE_MUTEX_HPP
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
#include <boost/date_time/posix_time/conversion.hpp>
#include <errno.h>
#include "timespec.hpp"

#ifdef _POSIX_TIMEOUTS
#if _POSIX_TIMEOUTS >= 0
#define BOOST_PTHREAD_HAS_TIMEDLOCK
#endif
#endif

namespace boost
{
    class recursive_mutex:
        boost::noncopyable
    {
    private:
        pthread_mutex_t m;
    public:
        recursive_mutex()
        {
            pthread_mutexattr_t attr;
            
            int const init_attr_res=pthread_mutexattr_init(&attr);
            if(init_attr_res)
            {
                throw thread_resource_error();
            }
            int const set_attr_res=pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
            if(set_attr_res)
            {
                throw thread_resource_error();
            }
            
            int const res=pthread_mutex_init(&m,&attr);
            if(res)
            {
                throw thread_resource_error();
            }
            int const destroy_attr_res=pthread_mutexattr_destroy(&attr);
            BOOST_ASSERT(!destroy_attr_res);
        }
        ~recursive_mutex()
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
        typedef unique_lock<recursive_mutex> scoped_lock;
        typedef scoped_lock scoped_try_lock;
    };

    typedef recursive_mutex recursive_try_mutex;

    class recursive_timed_mutex:
        boost::noncopyable
    {
    private:
        pthread_mutex_t m;
#ifndef BOOST_PTHREAD_HAS_TIMEDLOCK
        pthread_cond_t cond;
        bool is_locked;
        pthread_t owner;
        unsigned count;

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
        recursive_timed_mutex()
        {
#ifdef BOOST_PTHREAD_HAS_TIMEDLOCK
            pthread_mutexattr_t attr;
            
            int const init_attr_res=pthread_mutexattr_init(&attr);
            if(init_attr_res)
            {
                throw thread_resource_error();
            }
            int const set_attr_res=pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
            if(set_attr_res)
            {
                throw thread_resource_error();
            }
            
            int const res=pthread_mutex_init(&m,&attr);
            if(res)
            {
                int const destroy_attr_res=pthread_mutexattr_destroy(&attr);
                BOOST_ASSERT(!destroy_attr_res);
                throw thread_resource_error();
            }
            int const destroy_attr_res=pthread_mutexattr_destroy(&attr);
            BOOST_ASSERT(!destroy_attr_res);
#else
            int const res=pthread_mutex_init(&m,NULL);
            if(res)
            {
                throw thread_resource_error();
            }
            int const res2=pthread_cond_init(&cond,NULL);
            if(res2)
            {
                int const destroy_res=pthread_mutex_destroy(&m);
                BOOST_ASSERT(!destroy_res);
                throw thread_resource_error();
            }
            is_locked=false;
            count=0;
#endif
        }
        ~recursive_timed_mutex()
        {
            int const res=pthread_mutex_destroy(&m);
            BOOST_ASSERT(!res);
#ifndef BOOST_PTHREAD_HAS_TIMEDLOCK
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
            if(is_locked && owner==pthread_self())
            {
                ++count;
                return;
            }
            
            while(is_locked)
            {
                int const cond_res=pthread_cond_wait(&cond,&m);
                BOOST_ASSERT(!cond_res);
            }
            is_locked=true;
            ++count;
            owner=pthread_self();
        }

        void unlock()
        {
            pthread_mutex_scoped_lock const _(&m);
            if(!--count)
            {
                is_locked=false;
            }
            int const res=pthread_cond_signal(&cond);
            BOOST_ASSERT(!res);
        }
        
        bool try_lock()
        {
            pthread_mutex_scoped_lock const _(&m);
            if(is_locked && owner!=pthread_self())
            {
                return false;
            }
            is_locked=true;
            ++count;
            owner=pthread_self();
            return true;
        }

        bool timed_lock(system_time const & abs_time)
        {
            struct timespec const timeout=detail::get_timespec(abs_time);
            pthread_mutex_scoped_lock const _(&m);
            if(is_locked && owner==pthread_self())
            {
                ++count;
                return true;
            }
            while(is_locked)
            {
                int const cond_res=pthread_cond_timedwait(&cond,&m,&timeout);
                if(cond_res==ETIMEDOUT)
                {
                    return false;
                }
                BOOST_ASSERT(!cond_res);
            }
            is_locked=true;
            ++count;
            owner=pthread_self();
            return true;
        }
#endif

        typedef unique_lock<recursive_timed_mutex> scoped_timed_lock;
        typedef scoped_timed_lock scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
    };

}


#endif
