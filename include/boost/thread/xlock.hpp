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

#ifndef BOOST_XLOCK_WEK070601_HPP
#define BOOST_XLOCK_WEK070601_HPP

#include <boost/utility.hpp>
#include <boost/thread/exceptions.hpp>

namespace boost {

class condition;
struct xtime;

template <typename M>
class basic_lock : private noncopyable
{
public:
    friend class condition;
    
    typedef M mutex_type;
    
    explicit basic_lock(M& mx, bool lock_it=true)
        : m_mutex(mx), m_locked(false)
    {
        if (lock_it) lock();
    }
    ~basic_lock()
    {
        if (m_locked) unlock();
    }
    
    void lock()
    {
        if (m_locked) throw lock_error();
        m_mutex.do_lock();
        m_locked = true;
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        m_mutex.do_unlock();
        m_locked = false;
    }
    
    operator const void*() const { return m_locked ? this : 0; }
    
private:
    M& m_mutex;
    bool m_locked;
};

template <typename M>
class basic_trylock : private noncopyable
{
public:
    friend class condition;
    
    typedef M mutex_type;
    
    explicit basic_trylock(M& mx)
        : m_mutex(mx), m_locked(false)
    {
        try_lock();
    }
    basic_trylock(M& mx, bool lock_it)
        : m_mutex(mx), m_locked(false)
    {
        if (lock_it) lock();
    }
    ~basic_trylock()
    {
        if (m_locked) unlock();
    }
    
    void lock()
    {
        if (m_locked) throw lock_error();
        m_mutex.do_lock();
        m_locked = true;
    }
    bool try_lock()
    {
        if (m_locked) throw lock_error();
        return (m_locked = m_mutex.do_trylock());
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        m_mutex.do_unlock();
        m_locked = false;
    }
    
    operator const void*() const { return m_locked ? this : 0; }
    
private:
    M& m_mutex;
    bool m_locked;
};

template <typename M>
class basic_timedlock : private noncopyable
{
public:
    friend class condition;
    
    typedef M mutex_type;
    
    basic_timedlock(M& mx, const xtime& xt)
        : m_mutex(mx), m_locked(false)
    {
        timed_lock(xt);
    }
    basic_timedlock(M& mx, bool lock_it)
        : m_mutex(mx), m_locked(false)
    {
        if (lock_it) lock();
    }
    ~basic_timedlock()
    {
        if (m_locked) unlock();
    }
    
    void lock()
    {
        if (m_locked) throw lock_error();
        m_mutex.do_lock();
        m_locked = true;
    }
    bool timed_lock(const xtime& xt)
    {
        if (m_locked) throw lock_error();
        return (m_locked = m_mutex.do_timedlock(xt));
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        m_mutex.do_unlock();
        m_locked = false;
    }
    
    operator const void*() const { return m_locked ? this : 0; }
    
private:
    M& m_mutex;
    bool m_locked;
};

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//   22 May 01  WEKEMPF Modified to use xtime for time outs.

#endif // BOOST_XLOCK_WEK070601_HPP
