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
 
#ifndef BOOST_XLOCK_HPP
#define BOOST_XLOCK_HPP

#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility.hpp>

namespace boost {
    class condition;
    
    template <typename M>
    class basic_lock : noncopyable
    {
    public:
        friend class condition;
        
        typedef M mutex_type;
        
        explicit basic_lock(M& mx, bool lock_it=true)
            : _mutex(mx), _locked(false)
        {
            if (lock_it) lock();
        }
        ~basic_lock()
        {
            if (_locked) unlock();
        }
        
        void lock()
        {
            if (_locked) throw lock_error();
            _mutex.do_lock();
            _locked = true;
        }
        void unlock()
        {
            if (!_locked) throw lock_error();
            _mutex.do_unlock();
            _locked = false;
        }
        
        operator const void*() const { return _locked ? this : 0; }
        
    private:
        M& _mutex;
        bool _locked;
    };
    
    template <typename M>
    class basic_trylock : private noncopyable
    {
    public:
        friend class condition;
        
        typedef M mutex_type;
        
        explicit basic_trylock(M& mx)
            : _mutex(mx), _locked(false)
        {
            try_lock();
        }
        basic_trylock(M& mx, bool lock_it)
            : _mutex(mx), _locked(false)
        {
            if (lock_it) lock();
        }
        ~basic_trylock()
        {
            if (_locked) unlock();
        }
        
        void lock()
        {
            if (_locked) throw lock_error();
            _mutex.do_lock();
            _locked = true;
        }
        bool try_lock()
        {
            if (_locked) throw lock_error();
            return (_locked = _mutex.do_trylock());
        }
        void unlock()
        {
            if (!_locked) throw lock_error();
            _mutex.do_unlock();
            _locked = false;
        }
        
        operator const void*() const { return _locked ? this : 0; }
        
    private:
        M& _mutex;
        bool _locked;
    };
    
    template <typename M>
    class basic_timedlock : private noncopyable
    {
    public:
        friend class condition;
        
        typedef M mutex_type;
        
        basic_timedlock(M& mx, const xtime& xt)
            : _mutex(mx), _locked(false)
        {
            timed_lock(xt);
        }
        basic_timedlock(M& mx, bool lock_it)
            : _mutex(mx), _locked(false)
        {
            if (lock_it) lock();
        }
        ~basic_timedlock()
        {
            if (_locked) unlock();
        }
        
        void lock()
        {
            if (_locked) throw lock_error();
            _mutex.do_lock();
            _locked = true;
        }
        bool timed_lock(const xtime& xt)
        {
            if (_locked) throw lock_error();
            return (_locked = _mutex.do_timedlock(xt));
        }
        void unlock()
        {
            if (!_locked) throw lock_error();
            _mutex.do_unlock();
            _locked = false;
        }
        
        operator const void*() const { return _locked ? this : 0; }
        
    private:
        M& _mutex;
        bool _locked;
    };
} // namespace boost

#endif // BOOST_XLOCK_HPP
