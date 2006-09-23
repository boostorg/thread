// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2006 Roland Schwarz
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_RECURSIVE_MUTEX_RS06092302_HPP
#define BOOST_RECURSIVE_MUTEX_RS06092302_HPP

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

class BOOST_THREAD_DECL recursive_mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<recursive_mutex>;

    typedef detail::thread::scoped_lock<recursive_mutex> scoped_lock;

    recursive_mutex();
    ~recursive_mutex();

private:
    struct cv_state
    {
        long count;
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
    unsigned m_count;
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    pthread_cond_t m_unlocked;
    pthread_t m_thread_id;
    bool m_valid_id;
#endif
};

class BOOST_THREAD_DECL recursive_try_mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<recursive_try_mutex>;

    typedef detail::thread::scoped_lock<recursive_try_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<
        recursive_try_mutex> scoped_try_lock;

    recursive_try_mutex();
    ~recursive_try_mutex();

private:
    struct cv_state
    {
        long count;
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    bool do_trylock();
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
    unsigned m_count;
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    pthread_cond_t m_unlocked;
    pthread_t m_thread_id;
    bool m_valid_id;
#endif
};

class BOOST_THREAD_DECL recursive_timed_mutex
    : private noncopyable
{
public:
    friend class detail::thread::lock_ops<recursive_timed_mutex>;

    typedef detail::thread::scoped_lock<recursive_timed_mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<
        recursive_timed_mutex> scoped_try_lock;
    typedef detail::thread::scoped_timed_lock<
        recursive_timed_mutex> scoped_timed_lock;

    recursive_timed_mutex();
    ~recursive_timed_mutex();

private:
    struct cv_state
    {
        long count;
        pthread_mutex_t* pmutex;
    };
    void do_lock();
    bool do_trylock();
    bool do_timedlock(const xtime& xt);
    void do_unlock();
    void do_lock(cv_state& state);
    void do_unlock(cv_state& state);

    pthread_mutex_t m_mutex;
    pthread_cond_t m_unlocked;
    pthread_t m_thread_id;
    bool m_valid_id;
    unsigned m_count;
};

#ifdef BOOST_MSVC
#	pragma warning(pop)
#endif

} // namespace boost

#endif // BOOST_RECURSIVE_MUTEX_RS06092302_HPP
