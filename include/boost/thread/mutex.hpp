/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *    8 Feb 01  Initial version.
 *   22 May 01  Modified to use xtime for time outs.  Factored out
 *              to three classes, mutex, try_mutex and timed_mutex.
 */
 
#ifndef BOOST_MUTEX_HPP
#define BOOST_MUTEX_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/xlock.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost
{
    class condition;
    
    class mutex : private noncopyable
    {
    public:
        friend class basic_lock<mutex>;
        friend class condition;
        
        typedef basic_lock<mutex> lock;
        
        mutex();
        ~mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef void* cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            pthread_mutex_t* pmutex;
        };
#endif
        void do_lock();
        void do_unlock();
        void do_lock(cv_state& state);
        void do_unlock(cv_state& state);

#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _mutex;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
#endif
    };

    class try_mutex : private noncopyable
    {
    public:
        friend class basic_lock<try_mutex>;
        friend class basic_trylock<try_mutex>;
        friend class condition;
        
        typedef basic_lock<try_mutex> lock;
        typedef basic_trylock<try_mutex> trylock;
        
        try_mutex();
        ~try_mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef void* cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            pthread_mutex_t* pmutex;
        };
#endif
        void do_lock();
        bool do_trylock();
        void do_unlock();
        void do_lock(cv_state& state);
        void do_unlock(cv_state& state);

#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _mutex;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
#endif
    };

    class timed_mutex : private noncopyable
    {
    public:
        friend class basic_lock<timed_mutex>;
        friend class basic_trylock<timed_mutex>;
        friend class basic_timedlock<timed_mutex>;
        friend class condition;
        
        typedef basic_lock<timed_mutex> lock;
        typedef basic_trylock<timed_mutex> trylock;
        typedef basic_timedlock<timed_mutex> timedlock;
        
        timed_mutex();
        ~timed_mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef void* cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            pthread_mutex_t* pmutex;
        };
#endif
        void do_lock();
        bool do_trylock();
        bool do_timedlock(const xtime& xt);
        void do_unlock();
        void do_lock(cv_state& state);
        void do_unlock(cv_state& state);
        
#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _mutex;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
        pthread_cond_t _cond;
        bool _locked;
#endif
    };
} // namespace boost

#endif // BOOST_MUTEX_HPP
