#ifndef CONDITION_HPP
#define CONDITION_HPP
#include "boost/config.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/win32/thread_primitives.hpp"
#include "boost/thread/win32/xtime.hpp"
#include "boost/thread/win32/xtime_utils.hpp"
#include <limits.h>
#include "boost/assert.hpp"
#include <algorithm>

namespace boost
{
    template<typename lock_type>
    class basic_condition
    {
        boost::mutex internal_mutex;

        struct list_entry
        {
            detail::win32::handle semaphore;
            long count;
            bool notified;
        };

        BOOST_STATIC_CONSTANT(unsigned,generation_count=10);

        list_entry generations[generation_count];
        detail::win32::handle wake_sem;

        static bool no_waiters(list_entry const& entry)
        {
            return entry.count==0;
        }

        void shift_generations_down()
        {
            if(std::remove_if(generations,generations+generation_count,no_waiters)==generations+generation_count)
            {
                broadcast_entry(generations[generation_count-1],false);
            }
            std::copy_backward(generations,generations+generation_count,generations+generation_count);
            generations[0].semaphore=0;
            generations[0].count=0;
            generations[0].notified=false;
        }

        void broadcast_entry(list_entry& entry,bool wake)
        {
            if(wake)
            {
                detail::win32::ReleaseSemaphore(wake_sem,entry.count,NULL);
            }
            detail::win32::ReleaseSemaphore(entry.semaphore,entry.count,NULL);
            entry.count=0;
            dispose_entry(entry);
        }
        

        void dispose_entry(list_entry& entry)
        {
            BOOST_ASSERT(entry.count==0);
            if(entry.semaphore)
            {
                unsigned long const close_result=detail::win32::CloseHandle(entry.semaphore);
                BOOST_ASSERT(close_result);
            }
            entry.semaphore=0;
            entry.notified=false;
        }

        detail::win32::handle duplicate_handle(detail::win32::handle source)
        {
            detail::win32::handle const current_process=detail::win32::GetCurrentProcess();
            
            long const same_access_flag=2;
            detail::win32::handle new_handle=0;
            bool const success=detail::win32::DuplicateHandle(current_process,source,current_process,&new_handle,0,false,same_access_flag)!=0;
            BOOST_ASSERT(success);
            return new_handle;
        }

        bool do_wait(lock_type& lock,::boost::xtime const& target_time)
        {
            detail::win32::handle local_wake_sem;
            detail::win32::handle sem;
            bool first_loop=true;
            bool woken=false;
            while(!woken)
            {
                {
                    boost::mutex::scoped_lock internal_lock(internal_mutex);
                    if(first_loop)
                    {
                        lock.unlock();
                        if(!wake_sem)
                        {
                            wake_sem=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
                            BOOST_ASSERT(wake_sem);
                        }
                        local_wake_sem=duplicate_handle(wake_sem);
                        
                        if(generations[0].notified)
                        {
                            shift_generations_down();
                        }
                        if(!generations[0].semaphore)
                        {
                            generations[0].semaphore=detail::win32::create_anonymous_semaphore(0,LONG_MAX);
                            BOOST_ASSERT(generations[0].semaphore);
                        }
                        first_loop=false;
                    }
                    ++generations[0].count;
                    sem=duplicate_handle(generations[0].semaphore);
                }
                unsigned long const notified=detail::win32::WaitForSingleObject(sem,::boost::detail::get_milliseconds_until_time(target_time));
                BOOST_ASSERT(notified==detail::win32::timeout || notified==0);

                unsigned long const sem_close_result=detail::win32::CloseHandle(sem);
                BOOST_ASSERT(sem_close_result);

                if(notified==detail::win32::timeout)
                {
                    break;
                }
                
                unsigned long const woken_result=detail::win32::WaitForSingleObject(local_wake_sem,0);
                BOOST_ASSERT(woken_result==detail::win32::timeout || woken_result==0);

                woken=(woken_result==0);
            }
            unsigned long const wake_sem_close_result=detail::win32::CloseHandle(local_wake_sem);
            BOOST_ASSERT(wake_sem_close_result);
            lock.lock();
            return woken;
        }
        
    public:
        basic_condition():
            wake_sem(0)
        {
            for(unsigned i=0;i<generation_count;++i)
            {
                generations[i]=list_entry();
            }
        }
        
            
        ~basic_condition()
        {
            for(unsigned i=0;i<generation_count;++i)
            {
                dispose_entry(generations[i]);
            }
            detail::win32::CloseHandle(wake_sem);
        }

        void wait(lock_type& m)
        {
            do_wait(m,::boost::detail::get_xtime_sentinel());
        }

        template<typename predicate_type>
        void wait(lock_type& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }
        

        bool timed_wait(lock_type& m,::boost::xtime const& target_time)
        {
            return do_wait(m,target_time);
        }

        template<typename predicate_type>
        bool timed_wait(lock_type& m,::boost::xtime const& target_time,predicate_type pred)
        {
            while (!pred()) { if (!timed_wait(m, target_time)) return false; } return true;
        }
        
        void notify_one()
        {
            boost::mutex::scoped_lock internal_lock(internal_mutex);
            if(wake_sem)
            {
                detail::win32::ReleaseSemaphore(wake_sem,1,NULL);
                for(unsigned generation=generation_count;generation!=0;--generation)
                {
                    list_entry& entry=generations[generation-1];
                    if(entry.count)
                    {
                        entry.notified=true;
                        detail::win32::ReleaseSemaphore(entry.semaphore,1,NULL);
                        if(!--entry.count)
                        {
                            dispose_entry(entry);
                        }
                    }
                }
            }
        }
        
        void notify_all()
        {
            boost::mutex::scoped_lock internal_lock(internal_mutex);
            if(wake_sem)
            {
                for(unsigned generation=generation_count;generation!=0;--generation)
                {
                    list_entry& entry=generations[generation-1];
                    if(entry.count)
                    {
                        broadcast_entry(entry,true);
                    }
                }
            }
        }
        
    };
    
    typedef basic_condition<boost::mutex::scoped_lock> condition;
}

#endif
