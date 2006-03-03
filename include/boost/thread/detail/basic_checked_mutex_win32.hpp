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
#include <boost/thread/detail/win32_thread_primitives.hpp>
#include <boost/thread/detail/interlocked_read_win32.hpp>

namespace boost
{
    namespace detail
    {
        class mutex_error
        {};
            
        class mutex_deadlock:
            public mutex_error
        {};

        class mutex_recursion_error:
            public mutex_error
        {};

        class mutex_ownership_error:
            public mutex_error
        {};

        class mutex_unlock_error:
            public mutex_ownership_error
        {};
            
        class mutex_lifetime_error:
            public mutex_error
        {};

        class mutex_sync_object_error:
            public mutex_error
        {};

        class mutex_wait_error:
            public mutex_sync_object_error
        {};

        class mutex_close_error:
            public mutex_sync_object_error
        {};

        class mutex_release_error:
            public mutex_sync_object_error
        {};

        class mutex_create_error:
            public mutex_sync_object_error
        {};

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
                if(old_locking_thread)
                {
                    throw mutex_lifetime_error();
                }
                void* const old_semaphore=BOOST_INTERLOCKED_EXCHANGE_POINTER(&semaphore,0);
                if(old_semaphore)
                {
                    if(!BOOST_CLOSE_HANDLE(old_semaphore))
                    {
                        throw mutex_close_error();
                    }
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
                    if(BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),BOOST_INFINITE)!=0)
                    {
                        throw mutex_wait_error();
                    }
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
                if(old_locking_thread==destroyed_mutex_marker)
                {
                    throw mutex_lifetime_error();
                }
                else if(old_locking_thread!=current_thread)
                {
                    throw mutex_unlock_error();
                }

                long old_count=1;
                long current_count=0;
                while((current_count=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&active_count,old_count-1,old_count))!=old_count)
                {
                    old_count=current_count;
                }
                
                if(old_count!=1)
                {
                    if(!BOOST_RELEASE_SEMAPHORE(get_semaphore(),1,0))
                    {
                        throw mutex_release_error();
                    }
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
                if(current_locking_thread==current_thread)
                {
                    throw mutex_recursion_error();
                }
                else if(current_locking_thread==destroyed_mutex_marker)
                {
                    throw mutex_lifetime_error();
                }
            }
            
            void set_locking_thread()
            {
                if(BOOST_INTERLOCKED_COMPARE_EXCHANGE(&locking_thread,BOOST_GET_CURRENT_THREAD_ID(),0)!=0)
                {
                    throw mutex_ownership_error();
                }
            }
            
            void* get_semaphore()
            {
                void* current_semaphore=::boost::detail::interlocked_read(&semaphore);
                
                if(!current_semaphore)
                {
                    void* const new_semaphore=BOOST_CREATE_SEMAPHORE(0,0,1,0);
                    if(!new_semaphore)
                    {
                        throw mutex_create_error();
                    }
                    void* const old_semaphore=BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&semaphore,new_semaphore,0);
                    if(old_semaphore!=0)
                    {
                        if(!BOOST_CLOSE_HANDLE(new_semaphore))
                        {
                            throw mutex_close_error();
                        }
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
