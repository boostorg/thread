#ifndef BOOST_BASIC_TIMED_MUTEX_WIN32_HPP
#define BOOST_BASIC_TIMED_MUTEX_WIN32_HPP

//  basic_timed_mutex_win32.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/detail/win32_thread_primitives.hpp>
#include <boost/thread/detail/interlocked_read_win32.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/detail/xtime_utils.hpp>

namespace boost
{
    namespace detail
    {
        struct basic_timed_mutex
        {
            BOOST_STATIC_CONSTANT(long,lock_flag_value=0x10000);
            long active_count;
            void* semaphore;

            void initialize()
            {
                active_count=0;
                semaphore=0;
            }

            void destroy()
            {
                void* const old_semaphore=BOOST_INTERLOCKED_EXCHANGE_POINTER(&semaphore,0);
                if(old_semaphore)
                {
                    BOOST_CLOSE_HANDLE(old_semaphore);
                }
            }
            
          
            bool try_lock()
            {
                return !BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,lock_flag_value+1,0);
            }
            
            void lock()
            {
                BOOST_INTERLOCKED_INCREMENT(&active_count);
                while(true)
                {
                    if(try_set_lock_flag())
                    {
                        return;
                    }
                    BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),BOOST_INFINITE);
                }
            }
            bool timed_lock(::boost::xtime const& target_time)
            {
                BOOST_INTERLOCKED_INCREMENT(&active_count);
                while(true)
                {
                    if(try_set_lock_flag())
                    {
                        return true;
                    }
                    if(BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),::boost::detail::get_milliseconds_until_time(target_time))!=0)
                    {
                        BOOST_INTERLOCKED_DECREMENT(&active_count);
                        return false;
                    }
                }
            }

            long get_active_count()
            {
                return ::boost::detail::interlocked_read(&active_count);
            }

            void unlock()
            {
                long const offset=lock_flag_value+1;
                long old_count=BOOST_INTERLOCKED_EXCHANGE_ADD(&active_count,-offset);
                
                if(old_count>offset)
                {
                    BOOST_RELEASE_SEMAPHORE(get_semaphore(),1,0);
                }
            }

            bool locked()
            {
                return get_active_count()>=lock_flag_value;
            }
            
        private:
            bool try_set_lock_flag()
            {
                long new_count=0;
                while(new_count<lock_flag_value)
                {
                    long const count_before_exchange=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,new_count+lock_flag_value,new_count);
                    if(count_before_exchange==new_count)
                    {
                        return true;
                    }
                    new_count=count_before_exchange;
                }
                return false;
            }
            
            void* get_semaphore()
            {
                void* current_semaphore=::boost::detail::interlocked_read(&semaphore);
                
                if(!current_semaphore)
                {
                    void* const new_semaphore=BOOST_CREATE_SEMAPHORE(0,0,1,0);
                    void* const old_semaphore=BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&semaphore,new_semaphore,0);
                    if(old_semaphore!=0)
                    {
                        BOOST_CLOSE_HANDLE(new_semaphore);
                        return old_semaphore;
                    }
                    else
                    {
                        return new_semaphore;
                    }
                }
                return current_semaphore;
            }
            
        };
        
    }
}

#define BOOST_BASIC_TIMED_MUTEX_INITIALIZER {0}

#endif
