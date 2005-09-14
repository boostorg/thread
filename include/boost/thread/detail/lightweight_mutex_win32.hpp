#ifndef BOOST_LIGHTWEIGHT_MUTEX_WIN32_HPP
#define BOOST_LIGHTWEIGHT_MUTEX_WIN32_HPP

//  lightweight_mutex_win32.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/detail/win32_thread_primitives.hpp>

namespace boost
{
    namespace detail
    {
        class lightweight_mutex
        {
        public:
            long lock_flag;
            void* lock_sem;
            
            void initialize()
            {
                lock_flag=0;
                lock_sem=0;
            }
            
            ~lightweight_mutex()
            {
                BOOST_CLOSE_HANDLE(lock_sem);
            }

            bool locked()
            {
                return BOOST_INTERLOCKED_READ(&lock_flag);
            }
            
            void lock()
            {
                while(BOOST_INTERLOCKED_COMPARE_EXCHANGE(&lock_flag,1,0))
                {
                    BOOST_WAIT_FOR_SINGLE_OBJECT(get_semaphore(),BOOST_INFINITE);
                }
            }

            bool try_lock()
            {
                return !BOOST_INTERLOCKED_COMPARE_EXCHANGE(&lock_flag,1,0);
            }

            void unlock()
            {
                BOOST_INTERLOCKED_EXCHANGE(&lock_flag,0);
                void* const current_sem=BOOST_INTERLOCKED_READ_POINTER(&lock_sem);
                if(current_sem)
                {
                    BOOST_RELEASE_SEMAPHORE(current_sem,1,0);
                }
            }

            class scoped_lock
            {
                lightweight_mutex& m;
            public:
                
                scoped_lock(lightweight_mutex& m_):
                    m(m_)
                {
                    m.lock();
                }
                ~scoped_lock()
                {
                    m.unlock();
                }
            };

        private:
            void* get_semaphore()
            {
                void* current_semaphore=BOOST_INTERLOCKED_READ_POINTER(&lock_sem);
                if(!current_semaphore)
                {
                    void* const new_sem=BOOST_CREATE_SEMAPHORE(0,1,1,0);
                    current_semaphore=BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER(&lock_sem,new_sem,0);
                    if(current_semaphore)
                    {
                        CloseHandle(new_sem);
                    }
                    else
                    {
                        current_semaphore=new_sem;
                    }
                }
                return current_semaphore;
            }
            
        };
    }

#define BOOST_DETAIL_LIGHTWEIGHT_MUTEX_INIT {0}
}


#endif
