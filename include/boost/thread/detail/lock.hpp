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

    namespace detail { namespace thread {

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
            m_mutex.do_lock();
            m_locked = true;
        }
        void unlock()
        {
            if (!m_locked) throw lock_error();
            m_mutex.do_unlock();
            m_locked = false;
        }

        bool locked() const { return m_locked; }    
        operator const void*() const { return m_locked ? this : 0; }
    
    private:
        friend class boost::condition;
    
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
    
        bool locked() const { return m_locked; }    
        operator const void*() const { return m_locked ? this : 0; }
    
    private:
        friend class boost::condition;
    
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
    
        bool locked() const { return m_locked; }    
        operator const void*() const { return m_locked ? this : 0; }
    
    private:
        friend class boost::condition;
    
        TimedMutex& m_mutex;
        bool m_locked;
    };

    } // namespace thread
    } // namespace detail
} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//   22 May 01  WEKEMPF Modified to use xtime for time outs.
//   30 Jul 01  WEKEMPF Moved lock types into boost::detail::thread. Renamed some types.
//                      Added locked() methods.

#endif // BOOST_XLOCK_WEK070601_HPP
