// Copyright (C)  2002
// David Moore
//
// Original scoped_lock implementation
// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  David Moore makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_XRWLOCK_JDM031002_HPP
#define BOOST_XRWLOCK_JDM031002_HPP

#include <boost/utility.hpp>
#include <boost/thread/exceptions.hpp>

namespace boost {

class condition;
struct xtime;

typedef enum
{
    NO_LOCK=0,
    SHARED_LOCK=1,
    EXCL_LOCK=2
} rw_lock_state;

namespace detail { namespace thread {

template <typename Mutex>

    class rw_lock_ops : private noncopyable
    {
    private:
        rw_lock_ops() { }

    public:

        static void wrlock(Mutex& m)
        {
            m.do_wrlock();
        }
        static void rdlock(Mutex& m)
        {
            m.do_rdlock();
        }
        static void wrunlock(Mutex& m)
        {
            m.do_wrunlock();
        }
        static void rdunlock(Mutex &m)
        {
            m.do_rdunlock();
        }
        static bool try_wrlock(Mutex &m)
        {
            return m.do_try_wrlock();
        }
        static bool try_rdlock(Mutex &m)
        {
            return m.do_try_rdlock();
        }
       
        static bool timed_wrlock(Mutex &m,const xtime &xt)
        {
            return m.do_timed_wrlock(xt);
        }
        static bool timed_rdlock(Mutex &m,const xtime &xt)
        {
            return m.do_timed_rdlock(xt);
        }
    };


    template <typename RWMutex>
    class scoped_rw_lock : private noncopyable
    {
    public:
        typedef RWMutex mutex_type;

        explicit scoped_rw_lock(RWMutex& mx, rw_lock_state initial_state=SHARED_LOCK)
            : m_mutex(mx), m_locked(NO_LOCK)
        {
            if(initial_state == SHARED_LOCK)
                rdlock();
            else if(initial_state == EXCL_LOCK)
                wrlock();
        }
        ~scoped_rw_lock()
        {
            if(m_locked != NO_LOCK)
                unlock();

        }

        void rdlock()
        {
            if (m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<RWMutex>::rdlock(m_mutex);
            m_locked = SHARED_LOCK;
        }
        void wrlock()
        {
            if(m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<RWMutex>::wrlock(m_mutex);
            m_locked = EXCL_LOCK;
        }

        void unlock()
        {
            if (m_locked == NO_LOCK) throw lock_error();
            if(m_locked == SHARED_LOCK)
                rw_lock_ops<RWMutex>::rdunlock(m_mutex);
            else
                rw_lock_ops<RWMutex>::wrunlock(m_mutex);

            m_locked = NO_LOCK;
        }
      
        bool locked() const 
        { 
            return m_locked != NO_LOCK;
        }
        operator const void*() const 
        { 
            return (m_locked != NO_LOCK) ? this : 0; 
        }
        rw_lock_state state() const
        {
            return m_locked;
        }
        
    private:
        RWMutex& m_mutex;
        rw_lock_state m_locked;
    };


    template <typename TryRWMutex>
    class scoped_try_rw_lock : private noncopyable
    {
    public:
        typedef TryRWMutex mutex_type;
        
        explicit scoped_try_rw_lock(TryRWMutex& mx) 
            : m_mutex(mx), m_locked(NO_LOCK)
        {
            try_rdlock();
        }
        scoped_try_rw_lock(TryRWMutex& mx, rw_lock_state initial_state)
            : m_mutex(mx), m_locked(NO_LOCK)
        {
            if(initial_state == SHARED_LOCK)
                rdlock();
            else if(initial_state == EXCL_LOCK)
                wrlock();
        }
        ~scoped_try_rw_lock()
        {
            if(m_locked != NO_LOCK)
                unlock();
        }

        void rdlock()
        {
            if (m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<TryRWMutex>::rdlock(m_mutex);
            m_locked = SHARED_LOCK;
        }

        bool try_rdlock()
        {
            if (m_locked != NO_LOCK) throw lock_error();
            if(rw_lock_ops<TryRWMutex>::try_rdlock(m_mutex))
            {
                m_locked = SHARED_LOCK;
                return true;
            }
            return false;
        }
       
        void wrlock()
        {
            if(m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<TryRWMutex>::wrlock(m_mutex);
            m_locked = EXCL_LOCK;
        }

        bool try_wrlock()
        {
            if (m_locked != NO_LOCK) throw lock_error();
            if(rw_lock_ops<TryRWMutex>::try_wrlock(m_mutex))
            {
                m_locked = EXCL_LOCK;
                return true;
            }
            return false;
        }

        void unlock()
        {
            if (m_locked == NO_LOCK) throw lock_error();
            if(m_locked == SHARED_LOCK)
                rw_lock_ops<TryRWMutex>::rdunlock(m_mutex);
            else
                rw_lock_ops<TryRWMutex>::wrunlock(m_mutex);

            m_locked = NO_LOCK;
        }
      
        bool locked() const 
        { 
            return m_locked != NO_LOCK;
        }
        operator const void*() const 
        { 
            return (m_locked != NO_LOCK) ? this : 0; 
        }
         rw_lock_state state() const
        {
            return m_locked;
        }
    private:
        TryRWMutex& m_mutex;
        rw_lock_state m_locked;
    };


    template <typename TimedRWMutex>
    class scoped_timed_rw_lock : private noncopyable
    {
    public:
        typedef TimedRWMutex mutex_type;

        explicit scoped_timed_rw_lock(TimedRWMutex& mx, const xtime &xt) 
            : m_mutex(mx), m_locked(NO_LOCK)
        {
            timed_sharedlock(xt);
        }
        scoped_timed_rw_lock(TimedRWMutex& mx, rw_lock_state initial_state)
            : m_mutex(mx), m_locked(NO_LOCK)
        {
            if(initial_state == SHARED_LOCK)
                rdlock();
            else if(initial_state == EXCL_LOCK)
                wrlock();
        }
        ~scoped_timed_rw_lock()
        {
            if(m_locked != NO_LOCK)
                unlock();
        }

        void rdlock()
        {
            if (m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<TimedRWMutex>::rdlock(m_mutex);
            m_locked = SHARED_LOCK;
        }


        bool timed_rdlock(const xtime &xt)
        {
            if (m_locked != NO_LOCK) throw lock_error();
            if(rw_lock_ops<TimedRWMutex>::timed_rdlock(m_mutex,xt))
            {
                m_locked = SHARED_LOCK;
                return true;
            }
            return false;
        }

        void wrlock()
        {
            if(m_locked != NO_LOCK) throw lock_error();
            rw_lock_ops<TimedRWMutex>::wrlock(m_mutex);
            m_locked = EXCL_LOCK;
        }

        bool timed_wrlock(const xtime &xt)
        {
            if (m_locked != NO_LOCK) throw lock_error();
            if(rw_lock_ops<TimedRWMutex>::timed_wrlock(m_mutex,xt))
            {
                m_locked = EXCL_LOCK;
                return true;
            }
            return false;
        }


        void unlock()
        {
            if (m_locked == NO_LOCK) throw lock_error();
            if(m_locked == SHARED_LOCK)
                rw_lock_ops<TimedRWMutex>::rdunlock(m_mutex);
            else
                rw_lock_ops<TimedRWMutex>::wrunlock(m_mutex);

            m_locked = NO_LOCK;
        }
      
        bool locked() const 
        { 
            return m_locked != NO_LOCK;
        }
        operator const void*() const 
        { 
            return (m_locked != NO_LOCK) ? this : 0; 
        }
        rw_lock_state state() const
        {
            return m_locked;
        }
    private:
        TimedRWMutex& m_mutex;
        rw_lock_state m_locked;
    };

} // namespace thread
} // namespace detail
} // namespace boost

// Change Log:
//	03/10/02	Initial version


#endif
