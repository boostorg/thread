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

#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/thread/exceptions.hpp>
#include <boost/limits.hpp>
#include <cassert>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   define NOMINMAX
#   include <windows.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <errno.h>
#endif

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
condition::condition()
    : m_gone(0), m_blocked(0), m_waiting(0)
{
    m_gate = reinterpret_cast<unsigned long>(CreateSemaphore(0, 1, 1, 0));
    m_queue = reinterpret_cast<unsigned long>(CreateSemaphore(0, 0, std::numeric_limits<long>::max(), 0));
    m_mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));

    if (!m_gate || !m_queue || !m_mutex)
    {
        int res = 0;
		if (m_gate)
		{
			res = CloseHandle(reinterpret_cast<HANDLE>(m_gate));
			assert(res);
		}
		if (m_queue)
		{
			res = CloseHandle(reinterpret_cast<HANDLE>(m_queue));
			assert(res);
		}
		if (m_mutex)
		{
			res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
			assert(res);
		}

        throw thread_resource_error();
    }
}

condition::~condition()
{
    int res = 0;
    res = CloseHandle(reinterpret_cast<HANDLE>(m_gate));
    assert(res);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_queue));
    assert(res);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);
}

void condition::notify_one()
{
    unsigned signals = 0;

    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (m_waiting != 0) // the m_gate is already closed
    {
        if (m_blocked == 0)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
            assert(res);
            return;
        }

        ++m_waiting;
        --m_blocked;
    }
    else
    {
        res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
        assert(res == WAIT_OBJECT_0);
        if (m_blocked > m_gone)
        {
            if (m_gone != 0)
            {
                m_blocked -= m_gone;
                m_gone = 0;
            }
            signals = m_waiting = 1;
            --m_blocked;
        }
        else
        {
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
            assert(res);
        }

        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);

        if (signals)
        {
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_queue), signals, 0);
            assert(res);
        }
    }
}

void condition::notify_all()
{
    unsigned signals = 0;

    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (m_waiting != 0) // the m_gate is already closed
    {
        if (m_blocked == 0)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
            assert(res);
            return;
        }

        m_waiting += (signals = m_blocked);
        m_blocked = 0;
    }
    else
    {
        res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
        assert(res == WAIT_OBJECT_0);
        if (m_blocked > m_gone)
        {
            if (m_gone != 0)
            {
                m_blocked -= m_gone;
                m_gone = 0;
            }
            signals = m_waiting = m_blocked;
            m_blocked = 0;
        }
        else
        {
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
            assert(res);
        }

        res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
        assert(res);

        if (signals)
        {
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_queue), signals, 0);
            assert(res);
        }
    }
}

void condition::enter_wait()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
    assert(res == WAIT_OBJECT_0);
    ++m_blocked;
    res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
    assert(res);
}

void condition::do_wait()
{
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), INFINITE);
    assert(res == WAIT_OBJECT_0);
    
    unsigned was_waiting=0;
    unsigned was_gone=0;

    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);
    was_waiting = m_waiting;
    was_gone = m_gone;
    if (was_waiting != 0)
    {
        if (--m_waiting == 0)
        {
            if (m_blocked != 0)
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0); // open m_gate
                assert(res);
                was_waiting = 0;
            }
            else if (m_gone != 0)
                m_gone = 0;
        }
    }
    else if (++m_gone == (std::numeric_limits<unsigned>::max() / 2))
    {
        // timeout occured, normalize the m_gone count
        // this may occur if many calls to wait with a timeout are made and
        // no call to notify_* is made
        res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
        assert(res == WAIT_OBJECT_0);
        m_blocked -= m_gone;
        res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
        assert(res);
        m_gone = 0;
    }
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);

    if (was_waiting == 1)
    {
        for (/**/ ; was_gone; --was_gone)
        {
            // better now than spurious later
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), INFINITE);
            assert(res == WAIT_OBJECT_0);
        }
        res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
        assert(res);
    }
}

bool condition::do_timed_wait(const xtime& xt)
{
    unsigned milliseconds;
    to_duration(xt, milliseconds);

    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), milliseconds);
    assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

    bool ret = (res == WAIT_OBJECT_0);
    
    unsigned was_waiting=0;
    unsigned was_gone=0;

    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_mutex), INFINITE);
    assert(res == WAIT_OBJECT_0);
    was_waiting = m_waiting;
    was_gone = m_gone;
    if (was_waiting != 0)
    {
        if (!ret) // timeout
        {
            if (m_blocked != 0)
                --m_blocked;
            else
                ++m_gone; // count spurious wakeups
        }
        if (--m_waiting == 0)
        {
            if (m_blocked != 0)
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0); // open m_gate
                assert(res);
                was_waiting = 0;
            }
            else if (m_gone != 0)
                m_gone = 0;
        }
    }
    else if (++m_gone == (std::numeric_limits<unsigned>::max() / 2))
    {
        // timeout occured, normalize the m_gone count
        // this may occur if many calls to wait with a timeout are made and
        // no call to notify_* is made
        res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_gate), INFINITE);
        assert(res == WAIT_OBJECT_0);
        m_blocked -= m_gone;
        res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
        assert(res);
        m_gone = 0;
    }
    res = ReleaseMutex(reinterpret_cast<HANDLE>(m_mutex));
    assert(res);

    if (was_waiting == 1)
    {
        for (/**/ ; was_gone; --was_gone)
        {
            // better now than spurious later
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_queue), INFINITE);
            assert(res ==  WAIT_OBJECT_0);
        }
        res = ReleaseSemaphore(reinterpret_cast<HANDLE>(m_gate), 1, 0);
        assert(res);
    }

    return ret;
}
#elif defined(BOOST_HAS_PTHREADS)
condition::condition()
{
    int res = 0;
    res = pthread_cond_init(&m_condition, 0);
    if (res != 0)
        throw thread_resource_error();
}

condition::~condition()
{
    int res = 0;
    res = pthread_cond_destroy(&m_condition);
    assert(res == 0);
}

void condition::notify_one()
{
    int res = 0;
    res = pthread_cond_signal(&m_condition);
    assert(res == 0);
}

void condition::notify_all()
{
    int res = 0;
    res = pthread_cond_broadcast(&m_condition);
    assert(res == 0);
}

void condition::do_wait(pthread_mutex_t* pmutex)
{
    int res = 0;
    res = pthread_cond_wait(&m_condition, pmutex);
    assert(res == 0);
}

bool condition::do_timed_wait(const xtime& xt, pthread_mutex_t* pmutex)
{
    timespec ts;
    to_timespec(xt, ts);

    int res = 0;
    res = pthread_cond_timedwait(&m_condition, pmutex, &ts);
    assert(res == 0 || res == ETIMEDOUT);

    return res != ETIMEDOUT;
}
#endif

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//   22 May 01  WEKEMPF Modified to use xtime for time outs.
