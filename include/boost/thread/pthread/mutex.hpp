// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06040705_HPP
#define BOOST_THREAD_RS06040705_HPP

#include <boost/thread/pthread/config.hpp>

#include <boost/utility.hpp>
#include <boost/thread/pthread/lock.hpp>

#include <pthread.h>

namespace boost {

struct xtime;

// disable warnings about non dll import
// see: http://www.boost.org/more/separate_compilation.html#dlls
#ifdef BOOST_MSVC
#	pragma warning(push)
#	pragma warning(disable: 4251 4231 4660 4275)
#endif

class BOOST_THREAD_DECL mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<mutex>;

    typedef detail::thread::scoped_lock<mutex> scoped_lock;

    mutex();
    ~mutex();

private:
    struct cv_state
    {
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
};

class BOOST_THREAD_DECL try_mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<try_mutex>;

    typedef detail::thread::scoped_lock<try_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<try_mutex> scoped_try_lock;

    try_mutex();
    ~try_mutex();

private:
    struct cv_state
    {
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    bool do_trylock();
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
};

class BOOST_THREAD_DECL timed_mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<timed_mutex>;

    typedef detail::thread::scoped_lock<timed_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<timed_mutex> scoped_try_lock;
    typedef detail::thread::scoped_timed_lock<timed_mutex> scoped_timed_lock;

    timed_mutex();
    ~timed_mutex();

private:
    struct cv_state
    {
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    bool do_trylock();
    bool do_timedlock(const xtime& xt);
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
    pthread_cond_t m_condition;
    bool m_locked;
};

#ifdef BOOST_MSVC
#	pragma warning(pop)
#endif

} // namespace boost

#endif // BOOST_MUTEX_RS06040705_HPP
