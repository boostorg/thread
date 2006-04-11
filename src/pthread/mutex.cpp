// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>
#include <boost/thread/pthread/mutex.hpp>

#include <boost/thread/pthread/xtime.hpp>
#include <boost/thread/pthread/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/limits.hpp>
#include <string>
#include <stdexcept>
#include <cassert>
#include "timeconv.inl"

#include <errno.h>

namespace boost {

mutex::mutex()
{
    int res = 0;
    res = pthread_mutex_init(&m_mutex, 0);
    if (res != 0)
        throw thread_resource_error();
}

mutex::~mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);
}

void mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (res == EDEADLK) throw lock_error();
    assert(res == 0);
}

void mutex::do_unlock()
{
    int res = 0;
    res = pthread_mutex_unlock(&m_mutex);
    if (res == EPERM) throw lock_error();
    assert(res == 0);
}

void mutex::do_lock(cv_state&)
{
}

void mutex::do_unlock(cv_state& state)
{
    state.pmutex = &m_mutex;
}

try_mutex::try_mutex()
{
    int res = 0;
    res = pthread_mutex_init(&m_mutex, 0);
    if (res != 0)
        throw thread_resource_error();
}

try_mutex::~try_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);
}

void try_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (res == EDEADLK) throw lock_error();
    assert(res == 0);
}

bool try_mutex::do_trylock()
{
    int res = 0;
    res = pthread_mutex_trylock(&m_mutex);
    if (res == EDEADLK) throw lock_error();
    assert(res == 0 || res == EBUSY);
    return res == 0;
}

void try_mutex::do_unlock()
{
    int res = 0;
    res = pthread_mutex_unlock(&m_mutex);
    if (res == EPERM) throw lock_error();
    assert(res == 0);
}

void try_mutex::do_lock(cv_state&)
{
}

void try_mutex::do_unlock(cv_state& state)
{
    state.pmutex = &m_mutex;
}

timed_mutex::timed_mutex()
    : m_locked(false)
{
    int res = 0;
    res = pthread_mutex_init(&m_mutex, 0);
    if (res != 0)
        throw thread_resource_error();

    res = pthread_cond_init(&m_condition, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
}

timed_mutex::~timed_mutex()
{
    assert(!m_locked);
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

    res = pthread_cond_destroy(&m_condition);
    assert(res == 0);
}

void timed_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    while (m_locked)
    {
        res = pthread_cond_wait(&m_condition, &m_mutex);
        assert(res == 0);
    }

    assert(!m_locked);
    m_locked = true;

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

bool timed_mutex::do_trylock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    bool ret = false;
    if (!m_locked)
    {
        m_locked = true;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return ret;
}

bool timed_mutex::do_timedlock(const xtime& xt)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    timespec ts;
    to_timespec(xt, ts);

    while (m_locked)
    {
        res = pthread_cond_timedwait(&m_condition, &m_mutex, &ts);
        assert(res == 0 || res == ETIMEDOUT);

        if (res == ETIMEDOUT)
            break;
    }

    bool ret = false;
    if (!m_locked)
    {
        m_locked = true;
        ret = true;
    }

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return ret;
}

void timed_mutex::do_unlock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_locked);
    m_locked = false;

    res = pthread_cond_signal(&m_condition);
    assert(res == 0);

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

void timed_mutex::do_lock(cv_state&)
{
    int res = 0;
    while (m_locked)
    {
        res = pthread_cond_wait(&m_condition, &m_mutex);
        assert(res == 0);
    }

    assert(!m_locked);
    m_locked = true;

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

void timed_mutex::do_unlock(cv_state& state)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_locked);
    m_locked = false;

    res = pthread_cond_signal(&m_condition);
    assert(res == 0);

    state.pmutex = &m_mutex;
}

} // namespace boost

