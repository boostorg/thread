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
#include <boost/thread/recursive_mutex.hpp>
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

namespace boost {
#if defined(BOOST_HAS_WINTHREADS)
    recursive_mutex::recursive_mutex()
        : _count(0)
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::recursive_mutex : failure to construct");
    }

    recursive_mutex::~recursive_mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void recursive_mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        if (++_count > 1)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    void recursive_mutex::do_unlock()
    {
        if (--_count == 0)
        {
            int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    void recursive_mutex::do_lock(cv_state& state)
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        _count = state;
    }
    
    void recursive_mutex::do_unlock(cv_state& state)
    {
        state = _count;
        _count = 0;

        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }

    recursive_try_mutex::recursive_try_mutex()
        : _count(0)
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::recursive_try_mutex : failure to construct");
    }
    
    recursive_try_mutex::~recursive_try_mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void recursive_try_mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        if (++_count > 1)
        {
            res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    bool recursive_try_mutex::do_trylock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), 0);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

        if (res == WAIT_OBJECT_0)
        {
            if (+++_count > 1)
            {
                res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
                assert(res);
            }
            return true;
        }
        return false;
    }
    
    void recursive_try_mutex::do_unlock()
    {
        if (--_count == 0)
        {
            int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    void recursive_try_mutex::do_lock(cv_state& state)
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        _count = state;
    }
    
    void recursive_try_mutex::do_unlock(cv_state& state)
    {
        state = _count;
        _count = 0;

        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }

    recursive_timed_mutex::recursive_timed_mutex()
        : _count(0)
    {
        _mutex = reinterpret_cast<unsigned long>(CreateMutex(0, 0, 0));
        assert(_mutex);

        if (!_mutex)
            throw std::runtime_error("boost::recursive_timed_mutex : failure to construct");
    }

    recursive_timed_mutex::~recursive_timed_mutex()
    {
        int res = CloseHandle(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
    
    void recursive_timed_mutex::do_lock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        if (++_count > 1)
        {
            int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    bool recursive_timed_mutex::do_trylock()
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), 0);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

        if (res == WAIT_OBJECT_0)
        {
            if (+++_count > 1)
            {
                res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
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

        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), milliseconds);
        assert(res != WAIT_FAILED && res != WAIT_ABANDONED);

        if (res == WAIT_OBJECT_0)
        {
            if (+++_count > 1)
            {
                res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
                assert(res);
            }
            return true;
        }
        return false;
    }
    
    void recursive_timed_mutex::do_unlock()
    {
        if (--_count == 0)
        {
            int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
            assert(res);
        }
    }
    
    void recursive_timed_mutex::do_lock(cv_state& state)
    {
        int res = WaitForSingleObject(reinterpret_cast<HANDLE>(_mutex), INFINITE);
        assert(res == WAIT_OBJECT_0);

        _count = state;
    }
    
    void recursive_timed_mutex::do_unlock(cv_state& state)
    {
        state = _count;
        _count = 0;
        
        int res = ReleaseMutex(reinterpret_cast<HANDLE>(_mutex));
        assert(res);
    }
#elif defined(BOOST_HAS_PTHREADS)
    recursive_mutex::recursive_mutex()
        : _count(0)
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        , _valid_id(false)
#   endif
    {
        pthread_mutexattr_t attr;
        int res = pthread_mutexattr_init(&attr);
        assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        assert(res == 0);
#   endif

        res = pthread_mutex_init(&_mutex, &attr);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_mutex : failure to construct");

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_cond_init(&_unlocked, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_mutex : failure to construct");
#   endif
    }

    recursive_mutex::~recursive_mutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_cond_destroy(&_unlocked);
        assert(res == 0);
#   endif
    }
    
    void recursive_mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        if (++_count > 1)
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
        }
#   else
        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
            ++_count;
        else
        {
            while (_valid_id)
            {
                res = pthread_cond_wait(&_unlocked, &_mutex);
                assert(res == 0);
            }

            _thread_id = tid;
            _valid_id = true;
            _count = 1;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    void recursive_mutex::do_unlock()
    {
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        if (--_count == 0)
        {
            int res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
        }
#   else
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        pthread_t tid = pthread_self();
        if (_valid_id && !pthread_equal(_thread_id, tid))
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
            throw lock_error();
        }

        if (--_count == 0)
        {
            assert(_valid_id);
            _valid_id = false;

            res = pthread_cond_signal(&_unlocked);
            assert(res == 0);
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    void recursive_mutex::do_lock(cv_state& state)
    {
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        _count = state.count;
#   else
        int res;

        while (_valid_id)
        {
            res = pthread_cond_wait(&_unlocked, &_mutex);
            assert(res == 0);
        }

        _thread_id = pthread_self();
        _valid_id = true;
        _count = state.count;

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    void recursive_mutex::do_unlock(cv_state& state)
    {
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        assert(_valid_id);
        _valid_id = false;

        res = pthread_cond_signal(&_unlocked);
        assert(res == 0);
#   endif

        state.pmutex = &_mutex;
        state.count = _count;
    }

    recursive_try_mutex::recursive_try_mutex()
        : _count(0)
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        , _valid_id(false)
#   endif
    {
        pthread_mutexattr_t attr;
        int res = pthread_mutexattr_init(&attr);
        assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        assert(res == 0);
#   endif

        res = pthread_mutex_init(&_mutex, &attr);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_try_mutex : failure to construct");

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_cond_init(&_unlocked, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_try_mutex : failure to construct");
#   endif
    }

    recursive_try_mutex::~recursive_try_mutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);

#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        res = pthread_cond_destroy(&_unlocked);
        assert(res == 0);
#   endif
    }
    
    void recursive_try_mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        if (++_count > 1)
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
        }
#   else
        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
            ++_count;
        else
        {
            while (_valid_id)
            {
                res = pthread_cond_wait(&_unlocked, &_mutex);
                assert(res == 0);
            }

            _thread_id = tid;
            _valid_id = true;
            _count = 1;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    bool recursive_try_mutex::do_trylock()
    {
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        int res = pthread_mutex_trylock(&_mutex);
        assert(res == 0);

        if (res == 0)
        {
            if (++_count > 1)
            {
                res = pthread_mutex_unlock(&_mutex);
                assert(res == 0);
            }
            return true;
        }

        return false;
#   else
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        bool ret = false;
        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
        {
            ++_count;
            ret = true;
        }
        else if (!_valid_id)
        {
            _thread_id = tid;
            _valid_id = true;
            _count = 1;
            ret = true;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return ret;
#   endif
    }
    
    void recursive_try_mutex::do_unlock()
    {
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        if (--_count == 0)
        {
            int res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
        }
#   else
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        pthread_t tid = pthread_self();
        if (_valid_id && !pthread_equal(_thread_id, tid))
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
            throw lock_error();
        }

        if (--_count == 0)
        {
            assert(_valid_id);
            _valid_id = false;

            res = pthread_cond_signal(&_unlocked);
            assert(res == 0);
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    void recursive_try_mutex::do_lock(cv_state& state)
    {
#   if defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        _count = state.count;
#   else
        int res;

        while (_valid_id)
        {
            res = pthread_cond_wait(&_unlocked, &_mutex);
            assert(res == 0);
        }

        _thread_id = pthread_self();
        _valid_id = true;
        _count = state.count;

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
#   endif
    }
    
    void recursive_try_mutex::do_unlock(cv_state& state)
    {
#   if !defined(BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE)
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        assert(_valid_id);
        _valid_id = false;

        res = pthread_cond_signal(&_unlocked);
        assert(res == 0);
#   endif

        state.pmutex = &_mutex;
        state.count = _count;
    }

    recursive_timed_mutex::recursive_timed_mutex()
        : _valid_id(false), _count(0)
    {
        int res = pthread_mutex_init(&_mutex, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_timed_mutex : failure to construct");
        
        res = pthread_cond_init(&_unlocked, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::recursive_timed_mutex : failure to construct");
    }
    
    recursive_timed_mutex::~recursive_timed_mutex()
    {
        int res = pthread_mutex_destroy(&_mutex);
        assert(res == 0);

        res = pthread_cond_destroy(&_unlocked);
        assert(res == 0);
    }
    
    void recursive_timed_mutex::do_lock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
            ++_count;
        else
        {
            while (_valid_id)
            {
                res = pthread_cond_wait(&_unlocked, &_mutex);
                assert(res == 0);
            }

            _thread_id = tid;
            _valid_id = true;
            _count = 1;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    bool recursive_timed_mutex::do_trylock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        bool ret = false;
        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
        {
            ++_count;
            ret = true;
        }
        else if (!_valid_id)
        {
            _thread_id = tid;
            _valid_id = true;
            _count = 1;
            ret = true;
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return ret;
    }
    
    bool recursive_timed_mutex::do_timedlock(const xtime& xt)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        bool ret = false;
        pthread_t tid = pthread_self();
        if (_valid_id && pthread_equal(_thread_id, tid))
        {
            ++_count;
            ret = true;
        }
        else
        {
            timespec ts;
            to_timespec(xt, ts);

            while (_valid_id)
            {
                res = pthread_cond_timedwait(&_unlocked, &_mutex, &ts);
                if (res == ETIMEDOUT)
                    break;
                assert(res == 0);
            }
            
            if (!_valid_id)
            {
                _thread_id = tid;
                _valid_id = true;
                _count = 1;
                ret = true;
            }
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
        return ret;
    }
    
    void recursive_timed_mutex::do_unlock()
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        pthread_t tid = pthread_self();
        if (_valid_id && !pthread_equal(_thread_id, tid))
        {
            res = pthread_mutex_unlock(&_mutex);
            assert(res == 0);
            throw lock_error();
        }

        if (--_count == 0)
        {
            assert(_valid_id);
            _valid_id = false;

            res = pthread_cond_signal(&_unlocked);
            assert(res == 0);
        }

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    void recursive_timed_mutex::do_lock(cv_state& state)
    {
        int res;

        while (_valid_id)
        {
            res = pthread_cond_wait(&_unlocked, &_mutex);
            assert(res == 0);
        }

        _thread_id = pthread_self();
        _valid_id = true;
        _count = state.count;

        res = pthread_mutex_unlock(&_mutex);
        assert(res == 0);
    }
    
    void recursive_timed_mutex::do_unlock(cv_state& state)
    {
        int res = pthread_mutex_lock(&_mutex);
        assert(res == 0);

        assert(_valid_id);
        _valid_id = false;

        res = pthread_cond_signal(&_unlocked);
        assert(res == 0);

        state.pmutex = &_mutex;
        state.count = _count;
    }
#endif
} // namespace boost
