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

#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/limits.hpp>
#include <stdexcept>
#include <cassert>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <time.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <errno.h>
#endif

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
mutex::mutex()
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

mutex::~mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);
}

void mutex::do_unlock()
{
    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void mutex::do_lock(cv_state&)
{
    do_lock();
}

void mutex::do_unlock(cv_state&)
{
    do_unlock();
}

try_mutex::try_mutex()
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

try_mutex::~try_mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void try_mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);
}

bool try_mutex::do_trylock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), 0);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
    return res == WAIT_OBJECT_0;
}

void try_mutex::do_unlock()
{
    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void try_mutex::do_lock(cv_state&)
{
    do_lock();
}

void try_mutex::do_unlock(cv_state&)
{
    do_unlock();
}

timed_mutex::timed_mutex()
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

timed_mutex::~timed_mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void timed_mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);
}

bool timed_mutex::do_trylock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), 0);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
    return res == WAIT_OBJECT_0;
}

bool timed_mutex::do_timedlock(const xtime& xt)
{
    unsigned milliseconds;
    to_duration(xt, milliseconds);

    int res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), milliseconds);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
    return res == WAIT_OBJECT_0;
}

void timed_mutex::do_unlock()
{
    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void timed_mutex::do_lock(cv_state&)
{
    do_lock();
}

void timed_mutex::do_unlock(cv_state&)
{
    do_unlock();
}
#elif defined(BOOST_HAS_PTHREADS)
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

void mutex::do_lock(cv_state& state)
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

void try_mutex::do_lock(cv_state& state)
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

void timed_mutex::do_lock(cv_state& state)
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
#endif

} // namespace boost

// Change Log:
//   8 Feb 01  WEKEMPF Initial version.
