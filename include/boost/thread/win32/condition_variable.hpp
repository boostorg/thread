#ifndef BOOST_THREAD_CONDITION_VARIABLE_WIN32_HPP
#define BOOST_THREAD_CONDITION_VARIABLE_WIN32_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <boost/thread/mutex.hpp>
#include "thread_primitives.hpp"
#include <limits.h>
#include <boost/assert.hpp>
#include <algorithm>
#include <boost/thread/thread.hpp>
#include <boost/thread/thread_time.hpp>
#include "interlocked_read.hpp"
#include <boost/cstdint.hpp>
#include <boost/thread/xtime.hpp>

namespace boost
{
    namespace detail
    {
        class basic_condition_variable
        {
            boost::mutex internal_mutex;
            long total_count;
            unsigned active_generation_count;

            struct list_entry
            {
                detail::win32::handle semaphore;
                long count;
                bool notified;

                list_entry():
                    semaphore(0),count(0),notified(0)
                {}
            };

            BOOST_STATIC_CONSTANT(unsigned,generation_count=3);

            list_entry generations[generation_count];
            detail::win32::handle wake_sem;

            static bool no_waiters(list_entry const& entry)
            {
                return entry.count==0;
            }

            void shift_generations_down()
            {
                list_entry* const last_active_entry=std::remove_if(generations,generations+generation_count,no_waiters);
                if(last_active_entry==generations+generation_count)
                {
                    broadcast_entry(generations[generation_count-1],false);
                }
                else
                {
                    active_generation_count=(last_active_entry-generations)+1;
                }
            
                std::copy_backward(generations,generations+active_generation_count-1,generations+active_generation_count);
                generations[0]=list_entry();
            }

            void broadcast_entry(list_entry& entry,bool wake)
            {
                long const count_to_wake=entry.count;
                detail::interlocked_write_release(&total_count,total_count-count_to_wake);
                if(wake)
                {
                    detail::win32::ReleaseSemaphore(wake_sem,count_to_wake,0);
                }
                detail::win32::ReleaseSemaphore(entry.semaphore,count_to_wake,0);
                entry.count=0;
                dispose_entry(entry);
            }
        

            void dispose_entry(list_entry& entry)
            {
                if(entry.semaphore)
                {
                    BOOST_VERIFY(detail::win32::CloseHandle(entry.semaphore));
                    entry.semaphore=0;
                }
                entry.notified=false;
            }

            template<typename lock_type>
            struct relocker
            {
                lock_type& lock;
                bool unlocked;
                
                relocker(lock_type& lock_):
                    lock(lock_),unlocked(false)
                {}
                void unlock()
                {
                    lock.unlock();
                    unlocked=true;
                }
                ~relocker()
                {
                    if(unlocked)
                    {
                        lock.lock();
                    }
                    
                }
            };
            

        protected:
            struct timeout
            {
                unsigned long start;
                uintmax_t milliseconds;
                bool relative;
                boost::system_time abs_time;

                static unsigned long const max_non_infinite_wait=0xfffffffe;

                timeout(uintmax_t milliseconds_):
                    start(win32::GetTickCount()),
                    milliseconds(milliseconds_),
                    relative(true),
                    abs_time(boost::get_system_time())
                {}

                timeout(boost::system_time const& abs_time_):
                    start(win32::GetTickCount()),
                    milliseconds(0),
                    relative(false),
                    abs_time(abs_time_)
                {}

                struct remaining_time
                {
                    bool more;
                    unsigned long milliseconds;

                    remaining_time(uintmax_t remaining):
                        more(remaining>max_non_infinite_wait),
                        milliseconds(more?max_non_infinite_wait:(unsigned long)remaining)
                    {}
                };

                remaining_time remaining_milliseconds() const
                {
                    if(milliseconds==~uintmax_t(0))
                    {
                        return remaining_time(win32::infinite);
                    }
                    else if(relative)
                    {
                        unsigned long const now=win32::GetTickCount();
                        unsigned long const elapsed=now-start;
                        return remaining_time((elapsed<milliseconds)?(milliseconds-elapsed):0);
                    }
                    else
                    {
                        system_time const now=get_system_time();
                        if(abs_time<now)
                        {
                            return remaining_time(0);
                        }
                        return remaining_time((abs_time-get_system_time()).total_milliseconds()+1);
                    }
                }

                static timeout sentinel()
                {
                    return timeout(sentinel_type());
                }
            private:
                struct sentinel_type
                {};
                
                explicit timeout(sentinel_type):
                    start(0),milliseconds(~uintmax_t(0)),relative(true)
                {}
            };

            template<typename lock_type>
            bool do_wait(lock_type& lock,timeout wait_until)
            {
                detail::win32::handle_manager local_wake_sem;
                detail::win32::handle_manager sem;
                bool first_loop=true;
                bool woken=false;

                relocker<lock_type> locker(lock);
            
                while(!woken)
                {
                    {
                        boost::mutex::scoped_lock internal_lock(internal_mutex);
                        detail::interlocked_write_release(&total_count,total_count+1);
                        if(first_loop)
                        {
                            locker.unlock();
                            if(!wake_sem)
                            {
                                wake_sem=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
                                BOOST_ASSERT(wake_sem);
                            }
                            local_wake_sem=detail::win32::duplicate_handle(wake_sem);
                        
                            if(generations[0].notified)
                            {
                                shift_generations_down();
                            }
                            else if(!active_generation_count)
                            {
                                active_generation_count=1;
                            }
                        
                            first_loop=false;
                        }
                        if(!generations[0].semaphore)
                        {
                            generations[0].semaphore=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
                            BOOST_ASSERT(generations[0].semaphore);
                        }
                        ++generations[0].count;
                        sem=detail::win32::duplicate_handle(generations[0].semaphore);
                    }
                    while(true)
                    {
                        timeout::remaining_time const remaining=wait_until.remaining_milliseconds();
                        if(this_thread::interruptible_wait(sem,remaining.milliseconds))
                        {
                            break;
                        }
                        else if(!remaining.more)
                        {
                            return false;
                        }
                        if(wait_until.relative)
                        {
                            wait_until.milliseconds-=timeout::max_non_infinite_wait;
                        }
                    }
                
                    unsigned long const woken_result=detail::win32::WaitForSingleObject(local_wake_sem,0);
                    BOOST_ASSERT(woken_result==detail::win32::timeout || woken_result==0);

                    woken=(woken_result==0);
                }
                return woken;
            }

            template<typename lock_type,typename predicate_type>
            bool do_wait(lock_type& m,timeout const& wait_until,predicate_type pred)
            {
                while (!pred())
                {
                    if(!do_wait(m, wait_until))
                        return false;
                }
                return true;
            }
        
            basic_condition_variable(const basic_condition_variable& other);
            basic_condition_variable& operator=(const basic_condition_variable& other);
        public:
            basic_condition_variable():
                total_count(0),active_generation_count(0),wake_sem(0)
            {}
            
            ~basic_condition_variable()
            {
                for(unsigned i=0;i<generation_count;++i)
                {
                    dispose_entry(generations[i]);
                }
                detail::win32::CloseHandle(wake_sem);
            }

        
            void notify_one()
            {
                if(detail::interlocked_read_acquire(&total_count))
                {
                    boost::mutex::scoped_lock internal_lock(internal_mutex);
                    detail::win32::ReleaseSemaphore(wake_sem,1,0);
                    for(unsigned generation=active_generation_count;generation!=0;--generation)
                    {
                        list_entry& entry=generations[generation-1];
                        if(entry.count)
                        {
                            detail::interlocked_write_release(&total_count,total_count-1);
                            entry.notified=true;
                            detail::win32::ReleaseSemaphore(entry.semaphore,1,0);
                            if(!--entry.count)
                            {
                                dispose_entry(entry);
                                if(generation==active_generation_count)
                                {
                                    --active_generation_count;
                                }
                            }
                        }
                    }
                }
            }
        
            void notify_all()
            {
                if(detail::interlocked_read_acquire(&total_count))
                {
                    boost::mutex::scoped_lock internal_lock(internal_mutex);
                    for(unsigned generation=active_generation_count;generation!=0;--generation)
                    {
                        list_entry& entry=generations[generation-1];
                        if(entry.count)
                        {
                            broadcast_entry(entry,true);
                        }
                    }
                    active_generation_count=0;
                }
            }
        
        };
    }

    class condition_variable:
        public detail::basic_condition_variable
    {
    public:
        void wait(unique_lock<mutex>& m)
        {
            do_wait(m,timeout::sentinel());
        }

        template<typename predicate_type>
        void wait(unique_lock<mutex>& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }
        

        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& wait_until)
        {
            return do_wait(m,wait_until);
        }

        template<typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& wait_until,predicate_type pred)
        {
            return do_wait(m,wait_until,pred);
        }
        template<typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,boost::xtime const& wait_until,predicate_type pred)
        {
            return do_wait(m,system_time(wait_until),pred);
        }
        template<typename duration_type,typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,duration_type const& wait_duration,predicate_type pred)
        {
            return do_wait(m,wait_duration.total_milliseconds(),pred);
        }
    };
    
    class condition_variable_any:
        public detail::basic_condition_variable
    {
    public:
        template<typename lock_type>
        void wait(lock_type& m)
        {
            do_wait(m,timeout::sentinel());
        }

        template<typename lock_type,typename predicate_type>
        void wait(lock_type& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }
        
        template<typename lock_type>
        bool timed_wait(lock_type& m,boost::system_time const& wait_until)
        {
            return do_wait(m,wait_until);
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,boost::system_time const& wait_until,predicate_type pred)
        {
            return do_wait(m,wait_until,pred);
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,boost::xtime const& wait_until,predicate_type pred)
        {
            return do_wait(m,system_time(wait_until),pred);
        }

        template<typename lock_type,typename duration_type,typename predicate_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration,predicate_type pred)
        {
            return timed_wait(m,wait_duration.total_milliseconds(),pred);
        }
    };

}

#endif
