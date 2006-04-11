#ifndef BOOST_BASIC_CHECKED_MUTEX_WIN32_HPP
#define BOOST_BASIC_CHECKED_MUTEX_WIN32_HPP

//  basic_checked_mutex_win32.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/assert.hpp>

namespace boost
{
    namespace detail
    {
        struct basic_checked_mutex
        {
            long locking_thread;
            long active_count;
            void* semaphore;

            BOOST_STATIC_CONSTANT(long,destroyed_mutex_marker=~0);

            void initialize()
            {
                locking_thread=0;
                active_count=0;
                semaphore=0;
            }

            void destroy()
            {
                long const old_locking_thread=BOOST_INTERLOCKED_EXCHANGE(&locking_thread,destroyed_mutex_marker);
                BOOST_ASSERT(!old_locking_thread);
                void* const old_semaphore=BOOST_INTERLOCKED_EXCHANGE_POINTER(&semaphore,0);
                if(old_semaphore)
                {
                    bool const close_handle_succeeded=BOOST_CLOSE_HANDLE(old_semaphore)!=0;
                    BOOST_ASSERT(close_handle_succeeded);
                }
            }
          
            bool try_lock()
            {
                check_mutex_lock();
                bool const success=!BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,1,0);
                if(success)
                {
                    set_locking_thread();
                }
                    
                return success;
            }
            
            void lock()
            {
                check_mutex_lock();
                if(BOOST_INTERLOCKED_INCREMENT(&active_count)!=1)
                {
                    bool const wait_succeeded=BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),BOOST_INFINITE)==0;
                    BOOST_ASSERT(wait_succeeded);
                }
                set_locking_thread();
            }

            long get_active_count()
            {
                return ::boost::detail::interlocked_read(&active_count);
            }

            void unlock()
            {
                long const current_thread=BOOST_GET_CURRENT_THREAD_ID();
                long const old_locking_thread=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&locking_thread,0,current_thread);
                BOOST_ASSERT(old_locking_thread!=destroyed_mutex_marker);
                BOOST_ASSERT(old_locking_thread==current_thread);

                if(BOOST_INTERLOCKED_DECREMENT(&active_count)!=0)
                {
                    bool const release_succeeded=BOOST_RELEASE_SEMAPHORE(get_semaphore(),1,0)!=0;
                    BOOST_ASSERT(release_succeeded);
                }
            }

            bool locked()
            {
                return get_active_count()!=0;
            }
            
        private:
            void check_mutex_lock()
            {
                long const current_thread=BOOST_GET_CURRENT_THREAD_ID();
                long const current_locking_thread=::boost::detail::interlocked_read(&locking_thread);
                BOOST_ASSERT(current_locking_thread!=current_thread);
                BOOST_ASSERT(current_locking_thread!=destroyed_mutex_marker);
            }
            
            void set_locking_thread()
            {
                long const old_owner=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&locking_thread,BOOST_GET_CURRENT_THREAD_ID(),0);
                BOOST_ASSERT(old_owner==0);
            }
            
            void* get_semaphore()
            {
                void* current_semaphore=::boost::detail::interlocked_read(&semaphore);
                
                if(!current_semaphore)
                {
                    void* const new_semaphore=BOOST_CREATE_SEMAPHORE(0,0,1,0);
                    BOOST_ASSERT(new_semaphore);
                    void* const old_semaphore=BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&semaphore,new_semaphore,0);
                    if(old_semaphore!=0)
                    {
                        bool const close_succeeded=BOOST_CLOSE_HANDLE(new_semaphore)!=0;
                        BOOST_ASSERT(close_succeeded);
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

#define BOOST_BASIC_CHECKED_MUTEX_INITIALIZER {0}

#endif
