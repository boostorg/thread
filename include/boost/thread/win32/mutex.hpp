//  (C) Copyright 2005-6 Anthony Williams
// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06041004_HPP
#define BOOST_THREAD_RS06041004_HPP

#include <boost/thread/win32/config.hpp>

#ifdef BOOST_USE_CHECKED_MUTEX
#include <boost/thread/win32/basic_checked_mutex.hpp>
#else
#include <boost/thread/win32/basic_mutex.hpp>
#endif
#include <boost/thread/win32/basic_timed_mutex.hpp>
#include <boost/utility.hpp>
#include <boost/thread/win32/lock.hpp>
#include <boost/thread/win32/xtime.hpp>
#include <boost/thread/win32/exceptions.hpp>

namespace boost
{
    namespace detail
    {
#ifdef BOOST_USE_CHECKED_MUTEX
        typedef ::boost::detail::basic_checked_mutex underlying_mutex;
#else
        typedef ::boost::detail::basic_mutex underlying_mutex;
#endif
    }

    class mutex:
        noncopyable,
        protected ::boost::detail::underlying_mutex
    {
    public:
        mutex()
        {
            initialize();
        }
        ~mutex()
        {
            destroy();
        }

        class scoped_lock
        {
        protected:
            mutex& m;
            bool is_locked;
        public:
            scoped_lock(mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_lock(mutex& m_,bool do_lock):
                m(m_),is_locked(false)
            {
                if(do_lock)
                {
                    lock();
                }
            }
            ~scoped_lock()
            {
                if(locked())
                {
                    m.unlock();
                }
            }
            void lock()
            {
                if(locked())
                {
                    throw boost::lock_error();
                }
                m.lock();
                is_locked=true;
            }
            void unlock()
            {
                m.unlock();
                is_locked=false;
            }
            
            operator void* () const
            {
                return is_locked?const_cast<void*>(static_cast<void const*>(this)):0;
            }
            bool locked() const
            {
                return is_locked;
            }
        };
    };

    class try_mutex:
        noncopyable,
        protected ::boost::detail::underlying_mutex
    {
    public:
        try_mutex()
        {
            initialize();
        }
        ~try_mutex()
        {
            destroy();
        }

        class scoped_try_lock
        {
        protected:
            try_mutex& m;
            bool is_locked;
        public:
            scoped_try_lock(try_mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_try_lock(try_mutex& m_,bool do_lock):
                m(m_),is_locked(false)
            {
                if(do_lock)
                {
                    lock();
                }
            }
            ~scoped_try_lock()
            {
                if(locked())
                {
                    m.unlock();
                }
            }
            
            void lock()
            {
                m.lock();
                is_locked=true;
            }
            bool try_lock()
            {
                is_locked=m.try_lock();
                return is_locked;
            }
            void unlock()
            {
                m.unlock();
                is_locked=false;
            }
            
            operator void* () const
            {
                return is_locked?const_cast<void*>(static_cast<void const*>(this)):0;
            }
            bool locked() const
            {
                return is_locked;
            }
        };

        typedef scoped_try_lock scoped_lock;
    };

    class timed_mutex:
        noncopyable,
        protected ::boost::detail::basic_timed_mutex
    {
    public:
        timed_mutex()
        {
            initialize();
        }

        ~timed_mutex()
        {
            destroy();
        }

        class scoped_timed_lock
        {
        protected:
            timed_mutex& m;
            bool is_locked;
        public:
            scoped_timed_lock(timed_mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_timed_lock(timed_mutex& m_,::boost::xtime const& target):
                m(m_),is_locked(false)
            {
                timed_lock(target);
            }
            scoped_timed_lock(timed_mutex& m_,bool do_lock):
                m(m_),is_locked(false)
            {
                if(do_lock)
                {
                    lock();
                }
            }
            ~scoped_timed_lock()
            {
                if(locked())
                {
                    m.unlock();
                }
            }
            
            void lock()
            {
                m.lock();
                is_locked=true;
            }
            bool try_lock()
            {
                is_locked=m.try_lock();
                return is_locked;
            }
            bool timed_lock(::boost::xtime const& target)
            {
                is_locked=m.timed_lock(target);
                return is_locked;
            }
            
            void unlock()
            {
                m.unlock();
                is_locked=false;
            }
            
            operator void* () const
            {
                return is_locked?const_cast<void*>(static_cast<void const*>(this)):0;
            }
            bool locked() const
            {
                return is_locked;
            }
        };

        typedef scoped_timed_lock scoped_try_lock;
        typedef scoped_timed_lock scoped_lock;
    };
}

#endif // BOOST_THREAD_RS06041004_HPP
