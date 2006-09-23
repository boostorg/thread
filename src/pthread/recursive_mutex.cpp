// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2006 Roland Schwarz
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>
#include <boost/thread/pthread/recursive_mutex.hpp>

#include <boost/thread/pthread/xtime.hpp>
#include <boost/thread/pthread/thread.hpp>

#include <boost/limits.hpp>
#include <string>
#include <stdexcept>
#include <cassert>
#include <limits>

#include <errno.h>

namespace boost {

recursive_mutex::recursive_mutex()
    : m_count(0)
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    , m_valid_id(false)
#endif
{
    pthread_mutexattr_t attr;
    int res = pthread_mutexattr_init(&attr);
    assert(res == 0);

#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    assert(res == 0);
#endif

    res = pthread_mutex_init(&m_mutex, &attr);
    {
#       ifndef NDEBUG
        int res = 
#       endif		
	pthread_mutexattr_destroy(&attr);
        assert(res == 0);
    }
    if (res != 0)
        throw thread_resource_error();

#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_init(&m_unlocked, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
#endif
}

recursive_mutex::~recursive_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_destroy(&m_unlocked);
    assert(res == 0);
#endif
}

void recursive_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (++m_count > 1)
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#else
    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
        ++m_count;
    else
    {
        while (m_valid_id)
        {
            res = pthread_cond_wait(&m_unlocked, &m_mutex);
            assert(res == 0);
        }

        m_thread_id = tid;
        m_valid_id = true;
        m_count = 1;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

void recursive_mutex::do_unlock()
{
#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (--m_count == 0)
    {
        int res = 0;
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#else
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    pthread_t tid = pthread_self();
    if (m_valid_id && !pthread_equal(m_thread_id, tid))
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
        throw lock_error();
    }

    if (--m_count == 0)
    {
        assert(m_valid_id);
        m_valid_id = false;

        res = pthread_cond_signal(&m_unlocked);
        assert(res == 0);
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

void recursive_mutex::do_lock(cv_state& state)
{
#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    m_count = state.count;
#else
    int res = 0;

    while (m_valid_id)
    {
        res = pthread_cond_wait(&m_unlocked, &m_mutex);
        assert(res == 0);
    }

    m_thread_id = pthread_self();
    m_valid_id = true;
    m_count = state.count;

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

void recursive_mutex::do_unlock(cv_state& state)
{
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_valid_id);
    m_valid_id = false;

    res = pthread_cond_signal(&m_unlocked);
    assert(res == 0);
#endif

    state.pmutex = &m_mutex;
    state.count = m_count;
    m_count = 0;
}

recursive_try_mutex::recursive_try_mutex()
    : m_count(0)
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    , m_valid_id(false)
#endif
{
    pthread_mutexattr_t attr;
    int res = pthread_mutexattr_init(&attr);
    assert(res == 0);

#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    assert(res == 0);
#endif

    res = pthread_mutex_init(&m_mutex, &attr);
    {
#       ifndef NDEBUG
        int res = 
#       endif
	pthread_mutexattr_destroy(&attr);
        assert(res == 0);
    }
    if (res != 0)
        throw thread_resource_error();

#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_init(&m_unlocked, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
#endif
}

recursive_try_mutex::~recursive_try_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_destroy(&m_unlocked);
    assert(res == 0);
#endif
}

void recursive_try_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (++m_count > 1)
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#else
    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
        ++m_count;
    else
    {
        while (m_valid_id)
        {
            res = pthread_cond_wait(&m_unlocked, &m_mutex);
            assert(res == 0);
        }

        m_thread_id = tid;
        m_valid_id = true;
        m_count = 1;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

bool recursive_try_mutex::do_trylock()
{
#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_trylock(&m_mutex);
    assert(res == 0 || res == EBUSY);

    if (res == 0)
    {
        if (++m_count > 1)
        {
            res = pthread_mutex_unlock(&m_mutex);
            assert(res == 0);
        }
        return true;
    }

    return false;
#else
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    bool ret = false;
    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
    {
        ++m_count;
        ret = true;
    }
    else if (!m_valid_id)
    {
        m_thread_id = tid;
        m_valid_id = true;
        m_count = 1;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return ret;
#endif
}

void recursive_try_mutex::do_unlock()
{
#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (--m_count == 0)
    {
        int res = 0;
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#else
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    pthread_t tid = pthread_self();
    if (m_valid_id && !pthread_equal(m_thread_id, tid))
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
        throw lock_error();
    }

    if (--m_count == 0)
    {
        assert(m_valid_id);
        m_valid_id = false;

        res = pthread_cond_signal(&m_unlocked);
        assert(res == 0);
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

void recursive_try_mutex::do_lock(cv_state& state)
{
#if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    m_count = state.count;
#else
    int res = 0;

    while (m_valid_id)
    {
        res = pthread_cond_wait(&m_unlocked, &m_mutex);
        assert(res == 0);
    }

    m_thread_id = pthread_self();
    m_valid_id = true;
    m_count = state.count;

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
#endif
}

void recursive_try_mutex::do_unlock(cv_state& state)
{
#if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_valid_id);
    m_valid_id = false;

    res = pthread_cond_signal(&m_unlocked);
    assert(res == 0);
#endif

    state.pmutex = &m_mutex;
    state.count = m_count;
    m_count = 0;
}

recursive_timed_mutex::recursive_timed_mutex()
    : m_valid_id(false), m_count(0)
{
    int res = 0;
    res = pthread_mutex_init(&m_mutex, 0);
    if (res != 0)
        throw thread_resource_error();

    res = pthread_cond_init(&m_unlocked, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
}

recursive_timed_mutex::~recursive_timed_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

    res = pthread_cond_destroy(&m_unlocked);
    assert(res == 0);
}

void recursive_timed_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
        ++m_count;
    else
    {
        while (m_valid_id)
        {
            res = pthread_cond_wait(&m_unlocked, &m_mutex);
            assert(res == 0);
        }

        m_thread_id = tid;
        m_valid_id = true;
        m_count = 1;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

bool recursive_timed_mutex::do_trylock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    bool ret = false;
    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
    {
        ++m_count;
        ret = true;
    }
    else if (!m_valid_id)
    {
        m_thread_id = tid;
        m_valid_id = true;
        m_count = 1;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return ret;
}

bool recursive_timed_mutex::do_timedlock(const xtime& xt)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    bool ret = false;
    pthread_t tid = pthread_self();
    if (m_valid_id && pthread_equal(m_thread_id, tid))
    {
        ++m_count;
        ret = true;
    }
    else
    {
        boost::xtime t;
	// normalize, in case user has specified nsec only
	if (xt.nsec > 999999999)
	{
            t.sec  = xt.nsec / 1000000000;
	    t.nsec = xt.nsec % 1000000000;
	}
	else
	{
            t.sec = t.nsec = 0;
	}

        timespec ts;
	if (xt.sec < std::numeric_limits<time_t>::max() - t.sec) // avoid overflow
	{
            ts.tv_sec = static_cast<time_t>(xt.sec + t.sec);
	    ts.tv_nsec = static_cast<long>(t.nsec);
	}
	else
	{
            ts.tv_sec = std::numeric_limits<time_t>::max();
	    ts.tv_nsec = 999999999; // this should not overflow, or tv_nsec is odd anyways...
	}
//        to_timespec(xt, ts);

        while (m_valid_id)
        {
            res = pthread_cond_timedwait(&m_unlocked, &m_mutex, &ts);
            if (res == ETIMEDOUT)
                break;
            assert(res == 0);
        }

        if (!m_valid_id)
        {
            m_thread_id = tid;
            m_valid_id = true;
            m_count = 1;
            ret = true;
        }
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return ret;
}

void recursive_timed_mutex::do_unlock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    pthread_t tid = pthread_self();
    if (m_valid_id && !pthread_equal(m_thread_id, tid))
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
        throw lock_error();
    }

    if (--m_count == 0)
    {
        assert(m_valid_id);
        m_valid_id = false;

        res = pthread_cond_signal(&m_unlocked);
        assert(res == 0);
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

void recursive_timed_mutex::do_lock(cv_state& state)
{
    int res = 0;

    while (m_valid_id)
    {
        res = pthread_cond_wait(&m_unlocked, &m_mutex);
        assert(res == 0);
    }

    m_thread_id = pthread_self();
    m_valid_id = true;
    m_count = state.count;

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

void recursive_timed_mutex::do_unlock(cv_state& state)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_valid_id);
    m_valid_id = false;

    res = pthread_cond_signal(&m_unlocked);
    assert(res == 0);

    state.pmutex = &m_mutex;
    state.count = m_count;
    m_count = 0;
}

} // namespace boost
