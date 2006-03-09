#ifndef BOOST_BASIC_RECURSIVE_MUTEX_WIN32_HPP
#define BOOST_BASIC_RECURSIVE_MUTEX_WIN32_HPP

//  basic_recursive_mutex_win32.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/detail/win32_thread_primitives.hpp>
#include <boost/thread/detail/interlocked_read_win32.hpp>
#ifdef BOOST_USE_CHECKED_MUTEX
#include <boost/thread/detail/basic_checked_mutex_win32.hpp>
#else
#include <boost/thread/detail/basic_mutex_win32.hpp>
#endif
#include <boost/thread/detail/basic_timed_mutex_win32.hpp>
#include <boost/thread/xtime.hpp>

namespace boost
{
    namespace detail
    {
        template<typename underlying_mutex_type>
        struct basic_recursive_mutex_impl
        {
            long recursion_count;
            long locking_thread_id;
            underlying_mutex_type mutex;

            void initialize()
            {
                recursion_count=0;
                locking_thread_id=0;
                mutex.initialize();
            }

            void destroy()
            {
                mutex.destroy();
            }

            bool try_lock()
            {
                long const current_thread_id=BOOST_GET_CURRENT_THREAD_ID();
                return try_recursive_lock(current_thread_id) || try_basic_lock(current_thread_id);
            }
            
            void lock()
            {
                long const current_thread_id=BOOST_GET_CURRENT_THREAD_ID();
                if(!try_recursive_lock(current_thread_id))
                {
                    mutex.lock();
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                }
            }
            bool timed_lock(::boost::xtime const& target)
            {
                long const current_thread_id=BOOST_GET_CURRENT_THREAD_ID();
                return try_recursive_lock(current_thread_id) || try_timed_lock(current_thread_id,target);
            }
            long get_active_count()
            {
                return mutex.get_active_count();
            }

            void unlock()
            {
                if(!--recursion_count)
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,0);
                    mutex.unlock();
                }
            }

            bool locked()
            {
                return mutex.locked();
            }
            
        private:
            bool try_recursive_lock(long current_thread_id)
            {
                if(::boost::detail::interlocked_read(&locking_thread_id)==current_thread_id)
                {
                    ++recursion_count;
                    return true;
                }
                return false;
            }
            
            bool try_basic_lock(long current_thread_id)
            {
                if(mutex.try_lock())
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
            
            bool try_timed_lock(long current_thread_id,::boost::xtime const& target)
            {
                if(mutex.timed_lock(target))
                {
                    BOOST_INTERLOCKED_EXCHANGE(&locking_thread_id,current_thread_id);
                    recursion_count=1;
                    return true;
                }
                return false;
            }
            
        };

#ifdef BOOST_USE_CHECKED_MUTEX
        typedef basic_recursive_mutex_impl<basic_checked_mutex> basic_recursive_mutex;
#else
        typedef basic_recursive_mutex_impl<basic_mutex> basic_recursive_mutex;
#endif

        typedef basic_recursive_mutex_impl<basic_timed_mutex> basic_recursive_timed_mutex;
    }
}

#define BOOST_BASIC_RECURSIVE_MUTEX_INITIALIZER {0}

#endif
