#ifndef BOOST_RECURSIVE_MUTEX_WIN32_HPP
#define BOOST_RECURSIVE_MUTEX_WIN32_HPP

//  recursive_mutex.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)


#include <boost/thread/win32/basic_recursive_mutex.hpp>
#include <boost/utility.hpp>
#include <boost/thread/xtime.hpp>

namespace boost
{
    class recursive_mutex:
        noncopyable,
        protected ::boost::detail::basic_recursive_mutex
    {
    public:
        recursive_mutex()
        {
            ::boost::detail::basic_recursive_mutex::initialize();
        }
        ~recursive_mutex()
        {
            ::boost::detail::basic_recursive_mutex::destroy();
        }

        class scoped_lock
        {
        protected:
            recursive_mutex& m;
            bool is_locked;
        public:
            scoped_lock(recursive_mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_lock(recursive_mutex& m_,bool do_lock):
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
                if(!locked())
                {
                    throw boost::lock_error();
                }
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

    class recursive_try_mutex:
        noncopyable,
        protected ::boost::detail::basic_recursive_mutex
    {
    public:
        recursive_try_mutex()
        {
            ::boost::detail::basic_recursive_mutex::initialize();
        }
        ~recursive_try_mutex()
        {
            ::boost::detail::basic_recursive_mutex::destroy();
        }
        class scoped_try_lock
        {
        protected:
            recursive_try_mutex& m;
            bool is_locked;
        public:
            scoped_try_lock(recursive_try_mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_try_lock(recursive_try_mutex& m_,bool do_lock):
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
                if(locked())
                {
                    throw boost::lock_error();
                }
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
                if(!locked())
                {
                    throw boost::lock_error();
                }
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

    class recursive_timed_mutex:
        noncopyable,
        protected ::boost::detail::basic_recursive_timed_mutex
    {
    public:
        recursive_timed_mutex()
        {
            ::boost::detail::basic_recursive_timed_mutex::initialize();
        }
        ~recursive_timed_mutex()
        {
            ::boost::detail::basic_recursive_timed_mutex::destroy();
        }

        class scoped_timed_lock
        {
        protected:
            recursive_timed_mutex& m;
            bool is_locked;
        public:
            scoped_timed_lock(recursive_timed_mutex& m_):
                m(m_),is_locked(false)
            {
                lock();
            }
            scoped_timed_lock(recursive_timed_mutex& m_,::boost::xtime const& target):
                m(m_),is_locked(false)
            {
                timed_lock(target);
            }
            scoped_timed_lock(recursive_timed_mutex& m_,bool do_lock):
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


#endif
