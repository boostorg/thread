// Copyright (C) 2001-2003
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
#include <new>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <time.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <errno.h>
#elif defined(BOOST_HAS_MPTASKS)
#    include <MacErrors.h>

#    include "mac/init.hpp"
#    include "mac/safe.hpp"
#endif

namespace {

#if defined(BOOST_HAS_WINTHREADS)
class critsect_impl
{
public:
    critsect_impl()
    {
        InitializeCriticalSection(&m_critsect);
    }
    ~critsect_impl()
    {
        DeleteCriticalSection(&m_critsect);
    }
    void lock()
    {
        EnterCriticalSection(&m_critsect);
    }
    void unlock()
    {
        LeaveCriticalSection(&m_critsect);
    }

private:
    CRITICAL_SECTION m_critsect;
};

class mutex_impl
{
public:
    mutex_impl(const char* name=0)
    {
        m_mutex = CreateMutexA(0, 0, name);
        if (m_mutex == INVALID_HANDLE_VALUE)
            throw boost::thread_resource_error();
    }
    ~mutex_impl()
    {
        int res = 0;
        res = CloseHandle(m_mutex);
        assert(res);
    }
    void lock()
    {
        unsigned int res = 0;
        res = WaitForSingleObject(m_mutex, INFINITE);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
    }
    virtual bool trylock()
    {
        unsigned int res = 0;
        res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), 0);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
        return res == WAIT_OBJECT_0;
    }
    virtual bool timedlock(const boost::xtime& xt)
    {
        unsigned int res = 0;
        for (;;)
        {
            int milliseconds;
            to_duration(xt, milliseconds);

            res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex),
                milliseconds);
            assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

            if (res == WAIT_TIMEOUT)
            {
                boost::xtime cur;
                boost::xtime_get(&cur, boost::TIME_UTC);
                if (boost::xtime_cmp(xt, cur) > 0)
                    continue;
            }

            return res == WAIT_OBJECT_0;
        }
    }
    virtual void unlock()
    {
        int res = 0;
        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);
    }

private:
    HANDLE m_mutex;
};
#endif

} // namesapce

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
mutex::mutex()
{
    try
    {
        m_mutex = reinterpret_cast<void*>(new critsect_impl());
    }
    catch (std::bad_alloc&)
    {
        m_mutex = 0;
    }
    if (!m_mutex)
        throw thread_resource_error();
    m_critsect = true;
}

mutex::mutex(const char* name)
    : boost::detail::named_object(name)
{
    try
    {
        m_mutex = reinterpret_cast<void*>(new mutex_impl(
                                              effective_name().c_str()));
    }
    catch (std::bad_alloc&)
    {
        throw thread_resource_error();
    }
    m_critsect = false;
}

mutex::~mutex()
{
    if (m_critsect)
        delete reinterpret_cast<critsect_impl*>(m_mutex);
    else
        delete reinterpret_cast<mutex_impl*>(m_mutex);
}

void mutex::do_lock()
{
    if (m_critsect)
        reinterpret_cast<critsect_impl*>(m_mutex)->lock();
    else
        reinterpret_cast<mutex_impl*>(m_mutex)->lock();
}

void mutex::do_unlock()
{
    if (m_critsect)
        reinterpret_cast<critsect_impl*>(m_mutex)->unlock();
    else
        reinterpret_cast<mutex_impl*>(m_mutex)->unlock();
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
    try
    {
        m_mutex = reinterpret_cast<void*>(new mutex_impl(0));
    }
    catch (std::bad_alloc&)
    {
        throw thread_resource_error();
    }
}

try_mutex::try_mutex(const char* name)
    : boost::detail::named_object(name)
{
    try
    {
        m_mutex = reinterpret_cast<void*>(new mutex_impl(
                                              effective_name().c_str()));
    }
    catch (std::bad_alloc&)
    {
        throw thread_resource_error();
    }
}

try_mutex::~try_mutex()
{
	delete reinterpret_cast<mutex_impl*>(m_mutex);
}

void try_mutex::do_lock()
{
	reinterpret_cast<mutex_impl*>(m_mutex)->lock();
}

bool try_mutex::do_trylock()
{
	return reinterpret_cast<mutex_impl*>(m_mutex)->trylock();
}

void try_mutex::do_unlock()
{
	reinterpret_cast<mutex_impl*>(m_mutex)->unlock();
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
    try
    {
        m_mutex = reinterpret_cast<void*>(new mutex_impl(0));
    }
    catch (std::bad_alloc&)
    {
        throw thread_resource_error();
    }
}

timed_mutex::timed_mutex(const char* name)
    : boost::detail::named_object(name)
{
    try
    {
        m_mutex = reinterpret_cast<void*>(new mutex_impl(
                                              effective_name().c_str()));
    }
    catch (std::bad_alloc&)
    {
        throw thread_resource_error();
    }
}

timed_mutex::~timed_mutex()
{
	delete reinterpret_cast<mutex_impl*>(m_mutex);
}

void timed_mutex::do_lock()
{
	reinterpret_cast<mutex_impl*>(m_mutex)->lock();
}

bool timed_mutex::do_trylock()
{
	return reinterpret_cast<mutex_impl*>(m_mutex)->trylock();
}

bool timed_mutex::do_timedlock(const xtime& xt)
{
	return reinterpret_cast<mutex_impl*>(m_mutex)->timedlock(xt);
}

void timed_mutex::do_unlock()
{
	reinterpret_cast<mutex_impl*>(m_mutex)->unlock();
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
#elif defined(BOOST_HAS_MPTASKS)

using threads::mac::detail::safe_enter_critical_region;

mutex::mutex()
{
}

mutex::~mutex()
{
}

void mutex::do_lock()
{
    OSStatus lStatus = noErr;
    lStatus = safe_enter_critical_region(m_mutex, kDurationForever,
        m_mutex_mutex);
    assert(lStatus == noErr);
}

void mutex::do_unlock()
{
    OSStatus lStatus = noErr;
    lStatus = MPExitCriticalRegion(m_mutex);
    assert(lStatus == noErr);
}

void mutex::do_lock(cv_state& /*state*/)
{
    do_lock();
}

void mutex::do_unlock(cv_state& /*state*/)
{
    do_unlock();
}

try_mutex::try_mutex()
{
}

try_mutex::~try_mutex()
{
}

void try_mutex::do_lock()
{
    OSStatus lStatus = noErr;
    lStatus = safe_enter_critical_region(m_mutex, kDurationForever,
        m_mutex_mutex);
    assert(lStatus == noErr);
}

bool try_mutex::do_trylock()
{
    OSStatus lStatus = noErr;
    lStatus = MPEnterCriticalRegion(m_mutex, kDurationImmediate);
    assert(lStatus == noErr || lStatus == kMPTimeoutErr);
    return lStatus == noErr;
}

void try_mutex::do_unlock()
{
    OSStatus lStatus = noErr;
    lStatus = MPExitCriticalRegion(m_mutex);
    assert(lStatus == noErr);
}

void try_mutex::do_lock(cv_state& /*state*/)
{
    do_lock();
}

void try_mutex::do_unlock(cv_state& /*state*/)
{
    do_unlock();
}

timed_mutex::timed_mutex()
{
}

timed_mutex::~timed_mutex()
{
}

void timed_mutex::do_lock()
{
    OSStatus lStatus = noErr;
    lStatus = safe_enter_critical_region(m_mutex, kDurationForever,
        m_mutex_mutex);
    assert(lStatus == noErr);
}

bool timed_mutex::do_trylock()
{
    OSStatus lStatus = noErr;
    lStatus = MPEnterCriticalRegion(m_mutex, kDurationImmediate);
    assert(lStatus == noErr || lStatus == kMPTimeoutErr);
    return(lStatus == noErr);
}

bool timed_mutex::do_timedlock(const xtime& xt)
{
    int microseconds;
    to_microduration(xt, microseconds);
    Duration lDuration = kDurationMicrosecond * microseconds;

    OSStatus lStatus = noErr;
    lStatus = safe_enter_critical_region(m_mutex, lDuration, m_mutex_mutex);
    assert(lStatus == noErr || lStatus == kMPTimeoutErr);

    return(lStatus == noErr);
}

void timed_mutex::do_unlock()
{
    OSStatus lStatus = noErr;
    lStatus = MPExitCriticalRegion(m_mutex);
    assert(lStatus == noErr);
}

void timed_mutex::do_lock(cv_state& /*state*/)
{
    do_lock();
}

void timed_mutex::do_unlock(cv_state& /*state*/)
{
    do_unlock();
}
#endif

} // namespace boost

// Change Log:
//   8 Feb 01  WEKEMPF Initial version.
