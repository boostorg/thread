#ifndef BOOST_BASIC_TIMED_MUTEX_WIN32_HPP
#define BOOST_BASIC_TIMED_MUTEX_WIN32_HPP

//  basic_timed_mutex_win32.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/thread/win32/xtime.hpp>
#include <boost/thread/win32/xtime_utils.hpp>

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
                long old_count=0;
                while(!(old_count&lock_flag_value))
                {
                    long const current_count=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,(old_count+1)|lock_flag_value,old_count);
                    if(current_count==old_count)
                    {
                        return true;
                    }
                    old_count=current_count;
                }
                return false;
            }
            
            void lock()
            {
                bool const success=timed_lock(::boost::detail::get_xtime_sentinel());
                BOOST_ASSERT(success);
            }
            bool timed_lock(::boost::xtime const& target_time)
            {
                long old_count=0;
                while(true)
                {
                    long const current_count=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,(old_count+1)|lock_flag_value,old_count);
                    if(current_count==old_count)
                    {
                        break;
                    }
                    old_count=current_count;
                }

                if(old_count&lock_flag_value)
                {
                    bool lock_acquired=false;
                    void* const sem=get_semaphore();
                    ++old_count; // we're waiting, too
                    do
                    {
                        old_count-=(lock_flag_value+1); // there will be one less active thread on this mutex when it gets unlocked
                        if(BOOST_WAIT_FOR_SINGLE_OBJECT(sem,::boost::detail::get_milliseconds_until_time(target_time))!=0)
                        {
                            BOOST_INTERLOCKED_DECREMENT(&active_count);
                            return false;
                        }
                        do
                        {
                            long const current_count=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,old_count|lock_flag_value,old_count);
                            if(current_count==old_count)
                            {
                                break;
                            }
                            old_count=current_count;
                        }
                        while(!(old_count&lock_flag_value));
                        lock_acquired=!(old_count&lock_flag_value);
                    }
                    while(!lock_acquired);
                }
                return true;
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
