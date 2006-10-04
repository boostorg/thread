#ifndef BOOST_BASIC_MUTEX_WIN32_HPP
#define BOOST_BASIC_MUTEX_WIN32_HPP

//  basic_mutex_win32.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>

namespace boost
{
    namespace detail
    {
        struct basic_mutex
        {
            long lock_flag;
            void* semaphore;

            BOOST_STATIC_CONSTANT(long, unlocked = 0);
            BOOST_STATIC_CONSTANT(long, just_me = 1);
            BOOST_STATIC_CONSTANT(long, waiting_threads = 2);

            void initialize()
            {
                lock_flag=unlocked;
                semaphore=0;
                get_semaphore();
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
                return BOOST_INTERLOCKED_COMPARE_EXCHANGE(&lock_flag,just_me,unlocked)==unlocked;
            }
            
            void lock()
            {
                if(!try_lock())
                {
                    while(BOOST_INTERLOCKED_EXCHANGE(&lock_flag,waiting_threads)!=unlocked)
                    {
                        BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),BOOST_INFINITE);
                    }
                }
            }

            void unlock()
            {
                if(BOOST_INTERLOCKED_EXCHANGE(&lock_flag,unlocked)==waiting_threads)
                {
                    BOOST_RELEASE_SEMAPHORE(get_semaphore(),1,0);
                }
            }

            bool locked()
            {
                return ::boost::detail::interlocked_read(&lock_flag)!=unlocked;
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

#define BOOST_BASIC_MUTEX_INITIALIZER {0}

#endif
