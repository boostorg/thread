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
 
#include <boost/thread/semaphore.hpp>
#include <boost/thread/thread.hpp>
#include <ctime>
#include <cassert>
#include <limits>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#   include <boost/thread/mutex.hpp>
#   include <boost/thread/condition.hpp>
#endif

/*
 * Hack around various namespace challenged compilers
 */
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std {
    using ::clock_t;
    using ::clock;
} // namespace std
#endif

namespace boost {
#if defined(BOOST_HAS_WINTHREADS)
    semaphore::semaphore(unsigned count, unsigned max)
    {
        if (static_cast<long>(max) <= 0)
            max = std::numeric_limits<long>::max();

        _sema = reinterpret_cast<unsigned long>(CreateSemaphore(0, count, max, 0));
        assert(_sema != 0);

        if (!_sema)
            throw std::runtime_error("boost::semaphore : failure to construct");
    }
    
    semaphore::~semaphore()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_sema));
        assert(res);
    }
    
    bool semaphore::up(unsigned count, unsigned* prev)
    {
        long p;
        bool ret = !!ReleaseSemaphore(reinterpret_cast<HANDLE>(_sema), count, &p);
        assert(ret || GetLastError() == ERROR_TOO_MANY_POSTS);

        if (prev)
            *prev = p;

        return ret;
    }
    
    void semaphore::down()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_sema), INFINITE);
        assert(res == WAIT_OBJECT_0);
    }

    bool semaphore::down(const xtime& xt)
    {
        unsigned milliseconds;
        to_duration(xt, milliseconds);
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_sema), milliseconds);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
        return res == WAIT_OBJECT_0;
    }
#elif defined(BOOST_HAS_PTHREADS)
    semaphore::semaphore(unsigned count, unsigned max)
        : _available(count), _max(max ? max : std::numeric_limits<unsigned>::max())
    {
        int res = pthread_mutex_init(&_mutex, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::semaphore : failure to construct");

        res = pthread_cond_init(&_cond, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::semaphore : failure to construct");
    }
    
    semaphore::~semaphore()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);

        res = pthread_cond_destroy(&_cond);
        assert(res == 0);
    }
    
    bool semaphore::up(unsigned count, unsigned* prev)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        if (prev)
            *prev = _available;
        
        if (_available + count > _max)
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
            return false;
        }
        
        _available += count;

        res = pthread_cond_broadcast(&_cond);
        assert(res == 0);

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);        
        return true;
    }

    void semaphore::down()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        while (_available == 0)
        {
            res = pthread_cond_wait(&_cond, &_mutex);
            assert(res == 0);
        }

        _available--;
        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    bool semaphore::down(const xtime& xt)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        timespec ts;
        to_timespec(xt, ts);

        while (_available == 0)
        {
            res = pthread_cond_timedwait(&_cond, &_mutex, &ts);
            assert(res == 0 || res == ETIMEDOUT);

            if (res == ETIMEDOUT)
            {
                res = pthread_mutex_unlock(&_mutex);
                assert(res == 0);
                return false;
            }
        }

        _available--;
        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return true;
    }
#endif
} // namespace boost
