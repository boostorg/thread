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

#include <boost/thread/semaphore.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/limits.hpp>
#include <boost/thread/exceptions.hpp>
#include <stdexcept>
#include <cassert>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#   include <errno.h>
#   include <boost/thread/mutex.hpp>
#   include <boost/thread/condition.hpp>
#endif

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
semaphore::semaphore(unsigned count, unsigned max)
{
    if (static_cast<long>(max) <= 0)
        max = std::numeric_limits<long>::max();

    m_sema = reinterpret_cast<unsigned long>(CreateSemaphore(0, count, max, 0));
    if (!m_sema)
        throw thread_resource_error();
}

semaphore::~semaphore()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_sema));
    assert(res);
}

bool semaphore::up(unsigned count, unsigned* prev)
{
    long p;
    bool ret = !!ReleaseSemaphore(reinterpret_cast<HANDLE>(m_sema), count, &p);

    if (prev)
        *prev = p;

    return ret;
}

void semaphore::down()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_sema), INFINITE);
    assert(res == WAIT_OBJECT_0);
}

bool semaphore::down(const xtime& xt)
{
    unsigned milliseconds;
    to_duration(xt, milliseconds);
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_sema), milliseconds);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
    return res == WAIT_OBJECT_0;
}
#elif defined(BOOST_HAS_PTHREADS)
semaphore::semaphore(unsigned count, unsigned max)
    : m_available(count), m_max(max ? max : std::numeric_limits<unsigned>::max())
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

semaphore::~semaphore()
{
    int res = 0;
    res = pthread_mutex_destroy(&m_mutex);
    assert(res == 0);

    res = pthread_cond_destroy(&m_condition);
    assert(res == 0);
}

bool semaphore::up(unsigned count, unsigned* prev)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    if (prev)
        *prev = m_available;
    
    if (m_available + count > m_max)
    {
        res = pthread_mutex_unlock(&m_mutex);
        assert(res == 0);
        return false;
    }
    
    m_available += count;

    res = pthread_cond_broadcast(&m_condition);
    assert(res == 0);

    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);        
    return true;
}

void semaphore::down()
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    while (m_available == 0)
    {
        res = pthread_cond_wait(&m_condition, &m_mutex);
        assert(res == 0);
    }

    m_available--;
    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
}

bool semaphore::down(const xtime& xt)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    assert(res == 0);

    timespec ts;
    to_timespec(xt, ts);

    while (m_available == 0)
    {
        res = pthread_cond_timedwait(&m_condition, &m_mutex, &ts);
        assert(res == 0 || res == ETIMEDOUT);

        if (res == ETIMEDOUT)
        {
            res = pthread_mutex_unlock(&m_mutex);
            assert(res == 0);
            return false;
        }
    }

    m_available--;
    res = pthread_mutex_unlock(&m_mutex);
    assert(res == 0);
    return true;
}
#endif

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//   22 May 01  WEKEMPF Modified to use xtime for time outs.
