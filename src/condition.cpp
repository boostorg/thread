/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *    8 Feb 01  Initial version.
 *   22 May 01  Modified to use xtime for time outs.
 */

#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/thread/condition.hpp>
#include <limits>
#include <cassert>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <errno.h>
#endif

namespace boost
{
#if defined(BOOST_HAS_WINTHREADS)
    condition::condition()
        : _gone(0), _blocked(0), _waiting(0)
    {
        _gate = reinterpret_cast<unsigned long>(CreateSemaphore(0, 1, 1, 0));
        _queue = reinterpret_cast<unsigned long>(CreateSemaphore(0, 0, std::numeric_limits<long>::max(), 0));
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));

        if (!_gate || !_queue || !_mutex)
        {
            int res = CloseHandle(reinterpret_cast<HANDLE>(_gate));
            assert(res);
            res = CloseHandle(reinterpret_cast<HANDLE>(_queue));
            assert(res);
            res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
            assert(res);

            throw std::runtime_error("boost::condition : failure to construct");
        }
    }
    
    condition::~condition()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_gate));
        assert(res);
        res = CloseHandle(reinterpret_cast<HANDLE>(_queue));
        assert(res);
        res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }

    void condition::notify_one()
    {
        unsigned signals = 0;

        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        if (_waiting != 0) // the _gate is already closed
        {
            if (_blocked == 0)
            {
                res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
                assert(res);
                return;
            }

            ++_waiting;
            --_blocked = 0;
        }
        else
        {
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(_gate), INFINITE);
            assert(res == WAIT_OBJECT_0);
            if (_blocked > _gone)
            {
                if (_gone != 0)
                {
                    _blocked -= _gone;
                    _gone = 0;
                }
                signals = _waiting = 1;
                --_blocked;
            }
            else
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
                assert(res);
            }

            res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);

            if (signals)
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_queue), signals, 0);
                assert(res);
            }
        }
    }

    void condition::notify_all()
    {
        unsigned signals = 0;

        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        if (_waiting != 0) // the _gate is already closed
        {
            if (_blocked == 0)
            {
                res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
                assert(res);
                return;
            }

            _waiting += (signals = _blocked);
            _blocked = 0;
        }
        else
        {
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(_gate), INFINITE);
            assert(res == WAIT_OBJECT_0);
            if (_blocked > _gone)
            {
                if (_gone != 0)
                {
                    _blocked -= _gone;
                    _gone = 0;
                }
                signals = _waiting = _blocked;
                _blocked = 0;
            }
            else
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
                assert(res);
            }

            res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);

            if (signals)
            {
                res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_queue), signals, 0);
                assert(res);
            }
        }
    }

    void condition::enter_wait()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_gate), INFINITE);
        assert(res == WAIT_OBJECT_0);
        ++_blocked;
        res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
        assert(res);
    }

    void condition::do_wait()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_queue), INFINITE);
        assert(res == WAIT_OBJECT_0);
        
        unsigned was_waiting=0;
        unsigned was_gone=0;

        res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);
        was_waiting = _waiting;
        was_gone = _gone;
        if (was_waiting != 0)
        {
            if (--_waiting == 0)
            {
                if (_blocked != 0)
                {
                    res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0); // open _gate
                    assert(res);
                    was_waiting = 0;
                }
                else if (_gone != 0)
                    _gone = 0;
            }
        }
        else if (++_gone == (std::numeric_limits<unsigned>::max() / 2))
        {
            // timeout occured, normalize the _gone count
            // this may occur if many calls to wait with a timeout are made and
            // no call to notify_* is made
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(_gate), INFINITE);
            assert(res == WAIT_OBJECT_0);
            _blocked -= _gone;
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
            assert(res);
            _gone = 0;
        }
        res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);

        if (was_waiting == 1)
        {
            for (/**/ ; was_gone; --was_gone)
            {
                // better now than spurious later
                res = WaitForSingleObject(reinterpret_cast<HANDLE>(_queue), INFINITE);
                assert(res == WAIT_OBJECT_0);
            }
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
            assert(res);
        }
    }

    bool condition::do_timed_wait(const xtime& xt)
    {
        unsigned milliseconds;
        to_duration(xt, milliseconds);

        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_queue), milliseconds);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

        bool ret = (res == WAIT_OBJECT_0);
        
        unsigned was_waiting=0;
        unsigned was_gone=0;

        res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);
        was_waiting = _waiting;
        was_gone = _gone;
        if (was_waiting != 0)
        {
            if (!ret) // timeout
            {
                if (_blocked != 0)
                    --_blocked;
                else
                    ++_gone; // count spurious wakeups
            }
            if (--_waiting == 0)
            {
                if (_blocked != 0)
                {
                    res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0); // open _gate
                    assert(res);
                    was_waiting = 0;
                }
                else if (_gone != 0)
                    _gone = 0;
            }
        }
        else if (++_gone == (std::numeric_limits<unsigned>::max() / 2))
        {
            // timeout occured, normalize the _gone count
            // this may occur if many calls to wait with a timeout are made and
            // no call to notify_* is made
            res = WaitForSingleObject(reinterpret_cast<HANDLE>(_gate), INFINITE);
            assert(res == WAIT_OBJECT_0);
            _blocked -= _gone;
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
            assert(res);
            _gone = 0;
        }
        res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);

        if (was_waiting == 1)
        {
            for (/**/ ; was_gone; --was_gone)
            {
                // better now than spurious later
                res = WaitForSingleObject(reinterpret_cast<HANDLE>(_queue), INFINITE);
                assert(res ==  WAIT_OBJECT_0);
            }
            res = ReleaseSemaphore(reinterpret_cast<HANDLE>(_gate), 1, 0);
            assert(res);
        }

        return ret;
    }
#elif defined(BOOST_HAS_PTHREADS)
    condition::condition()
    {
        int res = pthread_cond_init(&_cond, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::condition : failure to construct");
    }
    
    condition::~condition()
    {
        int res = pthread_cond_destroy(&_cond);
        assert(res == 0);
    }

    void condition::notify_one()
    {
        int res = pthread_cond_signal(&_cond);
        assert(res == 0);
    }

    void condition::notify_all()
    {
        int res = pthread_cond_broadcast(&_cond);
        assert(res == 0);
    }

    void condition::do_wait(pthread_mutex_t* pmutex)
    {
        int res = pthread_cond_wait(&_cond, pmutex);
        assert(res == 0);
    }

    bool condition::do_timed_wait(const xtime& xt, pthread_mutex_t* pmutex)
    {
        timespec ts;
        to_timespec(xt, ts);

        int res = pthread_cond_timedwait(&_cond, pmutex, &ts);
        assert(res == 0 || res == ETIMEDOUT);

        return res != ETIMEDOUT;
    }
#endif
} // namespace boost
