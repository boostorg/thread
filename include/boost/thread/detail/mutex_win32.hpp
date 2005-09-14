#ifndef BOOST_MUTEX_WIN32_HPP
#define BOOST_MUTEX_WIN32_HPP

//  mutex_win32.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <boost/thread/detail/lightweight_mutex_win32.hpp>
#include <boost/utility.hpp>
#include <boost/thread/detail/lock.hpp>

namespace boost
{
    class mutex:
        noncopyable,
        protected ::boost::detail::lightweight_mutex
    {
    public:
        mutex()
        {
            lightweight_mutex::initialize();
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
        protected ::boost::detail::lightweight_mutex
    {
    public:
        try_mutex()
        {
            lightweight_mutex::initialize();
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
}


#endif
