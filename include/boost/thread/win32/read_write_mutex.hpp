#ifndef BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP
#define BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP

//  (C) Copyright 2006 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/static_assert.hpp>
#include <limits.h>

namespace boost
{
    class read_write_mutex
    {
    private:
        struct state_data
        {
            unsigned shared_count:10;
            unsigned shared_waiting:10;
            unsigned exclusive:1;
            unsigned upgradeable:1;
            unsigned exclusive_waiting:9;
            unsigned exclusive_waiting_blocked:1;

            friend bool operator==(state_data const& lhs,state_data const& rhs)
            {
                return *reinterpret_cast<unsigned const*>(&lhs)==*reinterpret_cast<unsigned const*>(&rhs);
            }
        };
        

        template<typename T>
        T interlocked_compare_exchange(T* target,T new_value,T comparand)
        {
            BOOST_STATIC_ASSERT(sizeof(T)==sizeof(long));
            long const res=BOOST_INTERLOCKED_COMPARE_EXCHANGE(reinterpret_cast<long*>(target),
                                                              *reinterpret_cast<long*>(&new_value),
                                                              *reinterpret_cast<long*>(&comparand));
            return *reinterpret_cast<T const*>(&res);
        }

        state_data state;
        void* semaphores[2];
        void* &unlock_sem;
        void* &exclusive_sem;
        void* upgradeable_sem;

        void release_waiters(state_data old_state)
        {
            if(old_state.exclusive_waiting)
            {
                bool const success=::boost::detail::ReleaseSemaphore(exclusive_sem,1,NULL)!=0;
                BOOST_ASSERT(success);
            }
                        
            if(old_state.shared_waiting || old_state.exclusive_waiting)
            {
                bool const success=::boost::detail::ReleaseSemaphore(unlock_sem,old_state.shared_waiting + (old_state.exclusive_waiting?1:0),NULL)!=0;
                BOOST_ASSERT(success);
            }
        }
        

    public:
        read_write_mutex():
            unlock_sem(semaphores[0]),
            exclusive_sem(semaphores[1]) 
        {
            unlock_sem=::boost::detail::CreateSemaphoreA(NULL,0,LONG_MAX,NULL);
            exclusive_sem=::boost::detail::CreateSemaphoreA(NULL,0,LONG_MAX,NULL);
            upgradeable_sem=::boost::detail::CreateSemaphoreA(NULL,0,LONG_MAX,NULL);
            state_data state_={0};
            state=state_;
        }

        ~read_write_mutex()
        {
            ::boost::detail::CloseHandle(upgradeable_sem);
            ::boost::detail::CloseHandle(unlock_sem);
            ::boost::detail::CloseHandle(exclusive_sem);
        }

        bool try_lock_shareable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                if(!new_state.exclusive && !new_state.exclusive_waiting_blocked)
                {
                    ++new_state.shared_count;
                }
                
                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(true);
            return !(old_state.exclusive| old_state.exclusive_waiting_blocked);
        }

        void lock_shareable()
        {
            while(true)
            {
                state_data old_state=state;
                do
                {
                    state_data new_state=old_state;
                    if(new_state.exclusive || new_state.exclusive_waiting_blocked)
                    {
                        ++new_state.shared_waiting;
                    }
                    else
                    {
                        ++new_state.shared_count;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }
                while(true);

                if(!(old_state.exclusive| old_state.exclusive_waiting_blocked))
                {
                    return;
                }
                    
                unsigned long const res=::boost::detail::WaitForSingleObject(unlock_sem,BOOST_INFINITE);
                BOOST_ASSERT(res==0);
            }
        }

        void unlock_shareable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                bool const last_reader=!--new_state.shared_count;
                
                if(last_reader)
                {
                    if(new_state.upgradeable)
                    {
                        new_state.upgradeable=false;
                        new_state.exclusive=true;
                    }
                    else
                    {
                        if(new_state.exclusive_waiting)
                        {
                            --new_state.exclusive_waiting;
                            new_state.exclusive_waiting_blocked=false;
                        }
                        new_state.shared_waiting=0;
                    }
                }
                
                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(last_reader)
                    {
                        if(old_state.upgradeable)
                        {
                            bool const success=::boost::detail::ReleaseSemaphore(upgradeable_sem,1,NULL)!=0;
                            BOOST_ASSERT(success);
                        }
                        else
                        {
                            release_waiters(old_state);
                        }
                    }
                    break;
                }
                old_state=current_state;
            }
            while(true);
        }

        void lock()
        {
            while(true)
            {
                state_data old_state=state;

                do
                {
                    state_data new_state=old_state;
                    if(new_state.shared_count || new_state.exclusive)
                    {
                        ++new_state.exclusive_waiting;
                        new_state.exclusive_waiting_blocked=true;
                    }
                    else
                    {
                        new_state.exclusive=true;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }
                while(true);

                if(!old_state.shared_count && !old_state.exclusive)
                {
                    break;
                }
                bool const success2=::boost::detail::WaitForMultipleObjects(2,semaphores,true,BOOST_INFINITE)<2;
                BOOST_ASSERT(success2);
            }
        }

        void unlock()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(true);
            release_waiters(old_state);
        }

        void lock_upgradeable()
        {
            while(true)
            {
                state_data old_state=state;
                do
                {
                    state_data new_state=old_state;
                    if(new_state.exclusive || new_state.exclusive_waiting_blocked || new_state.upgradeable)
                    {
                        ++new_state.shared_waiting;
                    }
                    else
                    {
                        ++new_state.shared_count;
                        new_state.upgradeable=true;
                    }

                    state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                    if(current_state==old_state)
                    {
                        break;
                    }
                    old_state=current_state;
                }
                while(true);

                if(!(old_state.exclusive|| old_state.exclusive_waiting_blocked|| old_state.upgradeable))
                {
                    return;
                }
                    
                unsigned long const res=::boost::detail::WaitForSingleObject(unlock_sem,BOOST_INFINITE);
                BOOST_ASSERT(res==0);
            }
        }

        void unlock_upgradeable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                new_state.upgradeable=false;
                bool const last_reader=!--new_state.shared_count;
                
                if(last_reader)
                {
                    if(new_state.exclusive_waiting)
                    {
                        --new_state.exclusive_waiting;
                        new_state.exclusive_waiting_blocked=false;
                    }
                    new_state.shared_waiting=0;
                }
                
                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(last_reader)
                    {
                        release_waiters(old_state);
                    }
                    break;
                }
                old_state=current_state;
            }
            while(true);
        }

        void unlock_upgradeable_and_lock()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                bool const last_reader=!--new_state.shared_count;
                
                if(last_reader)
                {
                    new_state.upgradeable=false;
                    new_state.exclusive=true;
                }
                
                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    if(!last_reader)
                    {
                        unsigned long const res=::boost::detail::WaitForSingleObject(upgradeable_sem,BOOST_INFINITE);
                        BOOST_ASSERT(res==0);
                    }
                    break;
                }
                old_state=current_state;
            }
            while(true);
        }

        void unlock_and_lock_upgradeable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                new_state.upgradeable=true;
                ++new_state.shared_count;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(true);
            release_waiters(old_state);
        }
        
        void unlock_and_lock_shareable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                new_state.exclusive=false;
                ++new_state.shared_count;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(true);
            release_waiters(old_state);
        }
        
        void unlock_upgradeable_and_lock_shareable()
        {
            state_data old_state=state;
            do
            {
                state_data new_state=old_state;
                new_state.upgradeable=false;
                if(new_state.exclusive_waiting)
                {
                    --new_state.exclusive_waiting;
                    new_state.exclusive_waiting_blocked=false;
                }
                new_state.shared_waiting=0;

                state_data const current_state=interlocked_compare_exchange(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(true);
            release_waiters(old_state);
        }
        
        class scoped_read_lock
        {
            read_write_mutex& m;
        public:
            scoped_read_lock(read_write_mutex& m_):
                m(m_)
            {
                m.lock_shareable();
            }
            ~scoped_read_lock()
            {
                m.unlock_shareable();
            }
        };

        class scoped_write_lock
        {
            read_write_mutex& m;
            bool locked;
            
        public:
            scoped_write_lock(read_write_mutex& m_):
                m(m_),locked(false)
            {
                lock();
            }
            void lock()
            {
                m.lock();
                locked=true;
            }
            
            void unlock()
            {
                m.unlock();
                locked=false;
            }
            ~scoped_write_lock()
            {
                if(locked)
                {
                    unlock();
                }
            }
        };

        class scoped_upgradeable_lock
        {
            read_write_mutex& m;
            bool locked;
            bool upgraded;
            
        public:
            scoped_upgradeable_lock(read_write_mutex& m_):
                m(m_),
                locked(false),upgraded(false)
            {
                lock();
            }
            void lock()
            {
                m.lock_upgradeable();
                locked=true;
            }
            void upgrade()
            {
                m.unlock_upgradeable_and_lock();
                upgraded=true;
            }
            void unlock()
            {
                if(upgraded)
                {
                    m.unlock();
                }
                else
                {
                    m.unlock_upgradeable();
                }
            }
            ~scoped_upgradeable_lock()
            {
                if(locked)
                {
                    unlock();
                }
            }
        };
        
        
    };
}


#endif
