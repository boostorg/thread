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

#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/xtime.hpp> 
#include <boost/thread/thread.hpp>
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
recursive_mutex::recursive_mutex()
    : m_count(0)
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

recursive_mutex::~recursive_mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void recursive_mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (++m_count > 1)
    {
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

void recursive_mutex::do_unlock()
{
    if (--m_count == 0)
    {
        int res = 0;
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

void recursive_mutex::do_lock(cv_state& state)
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    m_count = state;
}

void recursive_mutex::do_unlock(cv_state& state)
{
    state = m_count;
    m_count = 0;

    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

recursive_try_mutex::recursive_try_mutex()
    : m_count(0)
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

recursive_try_mutex::~recursive_try_mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void recursive_try_mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (++m_count > 1)
    {
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

bool recursive_try_mutex::do_trylock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), 0);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

    if (res == WAIT_OBJECT_0)
    {
        if (++m_count > 1)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
            assert(res);
        }
        return true;
    }
    return false;
}

void recursive_try_mutex::do_unlock()
{
    if (--m_count == 0)
    {
        int res = 0;
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

void recursive_try_mutex::do_lock(cv_state& state)
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    m_count = state;
}

void recursive_try_mutex::do_unlock(cv_state& state)
{
    state = m_count;
    m_count = 0;

    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

recursive_timed_mutex::recursive_timed_mutex()
    : m_count(0)
{
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
    if (!m_mutex)
        throw thread_resource_error();
}

recursive_timed_mutex::~recursive_timed_mutex()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void recursive_timed_mutex::do_lock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (++m_count > 1)
    {
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

bool recursive_timed_mutex::do_trylock()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), 0);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

    if (res == WAIT_OBJECT_0)
    {
        if (++m_count > 1)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
            assert(res);
        }
        return true;
    }
    return false;
}

bool recursive_timed_mutex::do_timedlock(const xtime& xt)
{
    unsigned milliseconds;
    to_duration(xt, milliseconds);

    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), milliseconds);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

    if (res == WAIT_OBJECT_0)
    {
        if (++m_count > 1)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
            assert(res);
        }
        return true;
    }
    return false;
}

void recursive_timed_mutex::do_unlock()
{
    if (--m_count == 0)
    {
        int res = 0;
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }
}

void recursive_timed_mutex::do_lock(cv_state& state)
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    m_count = state;
}

void recursive_timed_mutex::do_unlock(cv_state& state)
{
    state = m_count;
    m_count = 0;
    
    int res = 0;
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}
#elif defined(BOOST_HAS_PTHREADS)
recursive_mutex::recursive_mutex()
    : m_count(0)
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    , m_valid_id(false)
#   endif
{
    pthread_mutexattr_t attr;
    int res = 0;
    res = pthread_mutexattr_init(&attr);
    assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    assert(res == 0);
#   endif

    res = pthread_mutex_init(&m_mutex, &attr);
    if (res != 0)
        throw thread_resource_error();

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_init(&m_unlocked, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
#   endif
}

recursive_mutex::~recursive_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_destroy(&m_unlocked);
    assert(res == 0);
#   endif
}

void recursive_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (++m_count > 1)
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#   else
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
#   endif
}

void recursive_mutex::do_unlock()
{
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (--m_count == 0)
    {
        int res = 0;
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#   else
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
#   endif
}

void recursive_mutex::do_lock(cv_state& state)
{
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    m_count = state.count;
#   else
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
#   endif
}

void recursive_mutex::do_unlock(cv_state& state)
{
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_valid_id);
    m_valid_id = false;

    res = pthread_cond_signal(&m_unlocked);
    assert(res == 0);
#   endif

    state.pmutex = &m_mutex;
    state.count = m_count;
}

recursive_try_mutex::recursive_try_mutex()
    : m_count(0)
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    , m_valid_id(false)
#   endif
{
    pthread_mutexattr_t attr;
    int res = 0;
    res = pthread_mutexattr_init(&attr);
    assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    assert(res == 0);
#   endif

    res = pthread_mutex_init(&m_mutex, &attr);
    if (res != 0)
        throw thread_resource_error();

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_init(&m_unlocked, 0);
    if (res != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        throw thread_resource_error();
    }
#   endif
}

recursive_try_mutex::~recursive_try_mutex()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    res = pthread_cond_destroy(&m_unlocked);
    assert(res == 0);
#   endif
}

void recursive_try_mutex::do_lock()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (++m_count > 1)
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#   else
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
#   endif
}

bool recursive_try_mutex::do_trylock()
{
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_trylock(&m_mutex);
    assert(res == 0);

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
#   else
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
#   endif
}

void recursive_try_mutex::do_unlock()
{
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    if (--m_count == 0)
    {
        int res = 0;
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
    }
#   else
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
#   endif
}

void recursive_try_mutex::do_lock(cv_state& state)
{
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    m_count = state.count;
#   else
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
#   endif
}

void recursive_try_mutex::do_unlock(cv_state& state)
{
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    assert(m_valid_id);
    m_valid_id = false;

    res = pthread_cond_signal(&m_unlocked);
    assert(res == 0);
#   endif

    state.pmutex = &m_mutex;
    state.count = m_count;
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
        timespec ts;
        to_timespec(xt, ts);

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
}
#endif

} // namespace boost

// Change Log:
//   8 Feb 01  WEKEMPF Initial version.
