// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06041005_HPP
#define BOOST_THREAD_RS06041005_HPP

#include <boost/thread/win32/config.hpp>

#include <boost/utility.hpp>
#include <boost/thread/win32/exceptions.hpp>

namespace boost {

struct xtime;

namespace detail { namespace thread {

template <typename Mutex>
class lock_ops : private noncopyable
{
private:
    lock_ops() { }

public:
    typedef typename Mutex::cv_state lock_state;

    static void lock(Mutex& m)
    {
        m.do_lock();
    }
    static bool trylock(Mutex& m)
    {
        return m.do_trylock();
    }
    static bool timedlock(Mutex& m, const xtime& xt)
    {
        return m.do_timedlock(xt);
    }
    static void unlock(Mutex& m)
    {
        m.do_unlock();
    }
    static void lock(Mutex& m, lock_state& state)
    {
        m.do_lock(state);
    }
    static void unlock(Mutex& m, lock_state& state)
    {
        m.do_unlock(state);
    }
};

template <typename Mutex>
class scoped_lock : private noncopyable
{
public:
    typedef Mutex mutex_type;

    explicit scoped_lock(Mutex& mx, bool initially_locked=true)
        : m_mutex(mx), m_locked(false)
    {
        if (initially_locked) lock();
    }
    ~scoped_lock()
    {
        if (m_locked) unlock();
    }

    void lock()
    {
        if (m_locked) throw lock_error();
        lock_ops<Mutex>::lock(m_mutex);
        m_locked = true;
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        lock_ops<Mutex>::unlock(m_mutex);
        m_locked = false;
    }

    bool locked() const { return m_locked; }
    operator const void*() const { return m_locked ? this : 0; }

private:

    Mutex& m_mutex;
    bool m_locked;
};

template <typename TryMutex>
class scoped_try_lock : private noncopyable
{
public:
    typedef TryMutex mutex_type;

    explicit scoped_try_lock(TryMutex& mx)
        : m_mutex(mx), m_locked(false)
    {
        try_lock();
    }
    scoped_try_lock(TryMutex& mx, bool initially_locked)
        : m_mutex(mx), m_locked(false)
    {
        if (initially_locked) lock();
    }
    ~scoped_try_lock()
    {
        if (m_locked) unlock();
    }

    void lock()
    {
        if (m_locked) throw lock_error();
        lock_ops<TryMutex>::lock(m_mutex);
        m_locked = true;
    }
    bool try_lock()
    {
        if (m_locked) throw lock_error();
        return (m_locked = lock_ops<TryMutex>::trylock(m_mutex));
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        lock_ops<TryMutex>::unlock(m_mutex);
        m_locked = false;
    }

    bool locked() const { return m_locked; }
    operator const void*() const { return m_locked ? this : 0; }

private:

    TryMutex& m_mutex;
    bool m_locked;
};

template <typename TimedMutex>
class scoped_timed_lock : private noncopyable
{
public:
    typedef TimedMutex mutex_type;

    scoped_timed_lock(TimedMutex& mx, const xtime& xt)
        : m_mutex(mx), m_locked(false)
    {
        timed_lock(xt);
    }
    scoped_timed_lock(TimedMutex& mx, bool initially_locked)
        : m_mutex(mx), m_locked(false)
    {
        if (initially_locked) lock();
    }
    ~scoped_timed_lock()
    {
        if (m_locked) unlock();
    }

    void lock()
    {
        if (m_locked) throw lock_error();
        lock_ops<TimedMutex>::lock(m_mutex);
        m_locked = true;
    }
    bool try_lock()
    {
        if (m_locked) throw lock_error();
        return (m_locked = lock_ops<TimedMutex>::trylock(m_mutex));
    }
    bool timed_lock(const xtime& xt)
    {
        if (m_locked) throw lock_error();
        return (m_locked = lock_ops<TimedMutex>::timedlock(m_mutex, xt));
    }
    void unlock()
    {
        if (!m_locked) throw lock_error();
        lock_ops<TimedMutex>::unlock(m_mutex);
        m_locked = false;
    }

    bool locked() const { return m_locked; }
    operator const void*() const { return m_locked ? this : 0; }

private:

    TimedMutex& m_mutex;
    bool m_locked;
};

} // namespace thread
} // namespace detail
} // namespace boost

#endif // BOOST_THREAD_RS06041005_HPP
