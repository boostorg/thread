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
 *   8 Feb 01  Initial version.
 */

#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp> 
#include <boost/thread/mutex.hpp>
#include <ctime>
#include <limits>
#include <cassert>
#include "timeconv.inl"

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <time.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <errno.h>
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

namespace boost
{
#if defined(BOOST_HAS_WINTHREADS)
    mutex::mutex()
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::mutex : failure to construct");
    }

    mutex::~mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);
    }
    
    void mutex::do_unlock()
    {
        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void mutex::do_lock(cv_state& state)
    {
        do_lock();
    }
    
    void mutex::do_unlock(cv_state& state)
    {
        do_unlock();
    }

    try_mutex::try_mutex()
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::try_mutex : failure to construct");
    }

    try_mutex::~try_mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void try_mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);
    }
    
    bool try_mutex::do_trylock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), 0);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
        return res == WAIT_OBJECT_0;
    }
    
    void try_mutex::do_unlock()
    {
        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void try_mutex::do_lock(cv_state& state)
    {
        do_lock();
    }
    
    void try_mutex::do_unlock(cv_state& state)
    {
        do_unlock();
    }

    timed_mutex::timed_mutex()
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::timed_mutex : failure to construct");
    }

    timed_mutex::~timed_mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void timed_mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);
    }
    
    bool timed_mutex::do_trylock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), 0);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
        return res == WAIT_OBJECT_0;
    }
    
    bool timed_mutex::do_timedlock(const xtime& xt)
    {
        unsigned milliseconds;
        to_duration(xt, milliseconds);

        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), milliseconds);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);
        return res == WAIT_OBJECT_0;
    }
    
    void timed_mutex::do_unlock()
    {
        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void timed_mutex::do_lock(cv_state& state)
    {
        do_lock();
    }
    
    void timed_mutex::do_unlock(cv_state& state)
    {
        do_unlock();
    }
#elif defined(BOOST_HAS_PTHREADS)
    mutex::mutex()
    {
        int res = pthread_mutex_init(&_mutex, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::mutex : failure to construct");
    }
    
    mutex::~mutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);
    }
    
    void mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        if (res == EDEADLK) throw lock_error();
        assert(res == 0);
    }
    
    void mutex::do_unlock()
    {
        int res = pthread_mutex_unlock(&_mutex);
        if (res == EPERM) throw lock_error();
        assert(res == 0);
    }
    
    void mutex::do_lock(cv_state& state)
    {
    }
    
    void mutex::do_unlock(cv_state& state)
    {
        state.pmutex = &_mutex;
    }

    try_mutex::try_mutex()
    {
        int res = pthread_mutex_init(&_mutex, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::try_mutex : failure to construct");
    }
    
    try_mutex::~try_mutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);
    }
    
    void try_mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        if (res == EDEADLK) throw lock_error();
        assert(res == 0);
    }

    bool try_mutex::do_trylock()
    {
        int res = pthread_mutex_trylock(&_mutex);
        if (res == EDEADLK) throw lock_error();
        assert(res == 0 || res == EBUSY);
        return res == 0;
    }
    
    void try_mutex::do_unlock()
    {
        int res = pthread_mutex_unlock(&_mutex);
        if (res == EPERM) throw lock_error();
        assert(res == 0);
    }
    
    void try_mutex::do_lock(cv_state& state)
    {
    }
    
    void try_mutex::do_unlock(cv_state& state)
    {
        state.pmutex = &_mutex;
    }

    timed_mutex::timed_mutex()
        : _locked(false)
    {
        int res = pthread_mutex_init(&_mutex, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::timed_mutex : failure to construct");

        res = pthread_cond_init(&_cond, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::timed_mutex : failure to construct");
    }

    timed_mutex::~timed_mutex()
    {
        assert(!_locked);
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);

        res = pthread_cond_destroy(&_cond);
        assert(res == 0);
    }
    
    void timed_mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        while (_locked)
        {
            res = pthread_cond_wait(&_cond, &_mutex);
            assert(res == 0);
        }

        assert(!_locked);
        _locked = true;

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    bool timed_mutex::do_trylock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        bool ret = false;
        if (!_locked)
        {
            _locked = true;
            ret = true;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return ret;
    }
    
    bool timed_mutex::do_timedlock(const xtime& xt)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        timespec ts;
        to_timespec(xt, ts);

        while (_locked)
        {
            res = pthread_cond_timedwait(&_cond, &_mutex, &ts);
            assert(res == 0 || res == ETIMEDOUT);

            if (res == ETIMEDOUT)
                break;
        }
        
        bool ret = false;
        if (!_locked)
        {
            _locked = true;
            ret = true;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return ret;
    }
    
    void timed_mutex::do_unlock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        assert(_locked);
        _locked = false;

        res = pthread_cond_signal(&_cond);
        assert(res == 0);

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    void timed_mutex::do_lock(cv_state& state)
    {
        int res;
        while (_locked)
        {
            res = pthread_cond_wait(&_cond, &_mutex);
            assert(res == 0);
        }

        assert(!_locked);
        _locked = true;

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    void timed_mutex::do_unlock(cv_state& state)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        assert(_locked);
        _locked = false;

        res = pthread_cond_signal(&_cond);
        assert(res == 0);

        state.pmutex = &_mutex;
    }
#endif
} // namespace boost
