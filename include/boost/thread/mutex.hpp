// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_MUTEX_WEK070601_HPP
#define BOOST_MUTEX_WEK070601_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/detail/lock.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost {

struct xtime;

class mutex : private noncopyable
{
public:
    friend class detail::thread::lock_ops<mutex>;

    typedef detail::thread::scoped_lock<mutex> scoped_lock;

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
    void* m_mutex;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_mutex_t m_mutex;
#endif
};

class try_mutex : private noncopyable
{
public:
    friend class detail::thread::lock_ops<try_mutex>;

    typedef detail::thread::scoped_lock<try_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<try_mutex> scoped_try_lock;

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
    void* m_mutex;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_mutex_t m_mutex;
#endif
};

class timed_mutex : private noncopyable
{
public:
    friend class detail::thread::lock_ops<timed_mutex>;

    typedef detail::thread::scoped_lock<timed_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<timed_mutex> scoped_try_lock;
    typedef detail::thread::scoped_timed_lock<timed_mutex> scoped_timed_lock;

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
    void* m_mutex;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_mutex_t m_mutex;
    pthread_cond_t m_condition;
    bool m_locked;
#endif
};

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//   22 May 01  WEKEMPF Modified to use xtime for time outs.  Factored out
//                      to three classes, mutex, try_mutex and timed_mutex.

#endif // BOOST_MUTEX_WEK070601_HPP
