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
 *    1 Jun 01  Modified to use xtime for time outs.  Factored out
 *              to three classes, mutex, try_mutex and timed_mutex.
 *   11 Jun 01  Modified to use PTHREAD_MUTEX_RECURSIVE if available.
 */
 
#ifndef BOOST_RECURSIVE_MUTEX_HPP
#define BOOST_RECURSIVE_MUTEX_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/xlock.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost
{
    class condition;
    
    class recursive_mutex : private noncopyable
    {
    public:
        friend class basic_lock<recursive_mutex>;
        friend class condition;
        
        typedef basic_lock<recursive_mutex> lock;
        
        recursive_mutex();
        ~recursive_mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef size_t cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            long count;
            pthread_mutex_t* pmutex;
        };
#endif
        void do_lock();
        void do_unlock();
        void do_lock(cv_state& state);
        void do_unlock(cv_state& state);

#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _mutex;
        unsigned long _count;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
        unsigned _count;
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        pthread_cond_t _unlocked;
        pthread_t _thread_id;
        bool _valid_id;
#   endif
#endif
    };

    class recursive_try_mutex : private noncopyable
    {
    public:
        friend class basic_lock<recursive_try_mutex>;
        friend class basic_trylock<recursive_try_mutex>;
        friend class condition;
        
        typedef basic_lock<recursive_try_mutex> lock;
        typedef basic_trylock<recursive_try_mutex> trylock;
        
        recursive_try_mutex();
        ~recursive_try_mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef size_t cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            long count;
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
        unsigned long _count;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
        unsigned _count;
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        pthread_cond_t _unlocked;
        pthread_t _thread_id;
        bool _valid_id;
#   endif
#endif
    };

    class recursive_timed_mutex : private noncopyable
    {
    public:
        friend class basic_lock<recursive_timed_mutex>;
        friend class basic_trylock<recursive_timed_mutex>;
        friend class basic_timedlock<recursive_timed_mutex>;
        friend class condition;
        
        typedef basic_lock<recursive_timed_mutex> lock;
        typedef basic_trylock<recursive_timed_mutex> trylock;
        typedef basic_timedlock<recursive_timed_mutex> timedlock;
        
        recursive_timed_mutex();
        ~recursive_timed_mutex();
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        typedef size_t cv_state;
#elif defined(BOOST_HAS_PTHREADS)
        struct cv_state
        {
            long count;
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
        unsigned long _count;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
        pthread_cond_t _unlocked;
        pthread_t _thread_id;
        bool _valid_id;
        unsigned _count;
#endif
    };
} // namespace boost

#endif // BOOST_RECURSIVE_MUTEX_HPP
