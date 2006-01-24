#ifndef BOOST_CONDITION_WIN32_HPP
#define BOOST_CONDITION_WIN32_HPP

//  condition_win32.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/detail/interlocked.hpp>
#include <boost/thread/detail/win32_thread_primitives.hpp>
#include <boost/thread/detail/lightweight_mutex_win32.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/detail/xtime_utils.hpp>
#include <boost/thread/detail/interlocked_read_win32.hpp>
#include <limits.h>

namespace boost
{
    class condition
    {
    public:
        typedef ::boost::detail::lightweight_mutex gate_type;
        gate_type state_change_gate;
        typedef gate_type::scoped_lock gate_scoped_lock;
        void* notification_sem;
        long waiting_count;
        long notify_count;

    private:
        
        void release_notification_sem(bool release_all)
        {
            state_change_gate.lock();
            long const waiters=::boost::detail::interlocked_read(&waiting_count);
            if(waiters)
            {
                long const count_to_unlock=release_all?waiters:1;
                BOOST_INTERLOCKED_EXCHANGE(&notify_count,count_to_unlock);
                BOOST_RELEASE_SEMAPHORE(notification_sem,count_to_unlock,0);
            }
            else
            {
                state_change_gate.unlock();
            }
        }

        struct once_predicate
        {
            bool called_before;
            once_predicate():
                called_before(false)
            {}
            
            bool operator()()
            {
                if(!called_before)
                {
                    called_before=true;
                    return false;
                }
                return true;
            }
        };
        
        template<typename scoped_lock_type>
        bool do_wait(scoped_lock_type& m,unsigned time_to_wait_in_milliseconds=BOOST_INFINITE)
        {
            {
                gate_scoped_lock lock(state_change_gate);
                BOOST_INTERLOCKED_INCREMENT(&waiting_count);
                m.unlock();
            }
            
            bool const notified=BOOST_WAIT_FOR_SINGLE_OBJECT(notification_sem,time_to_wait_in_milliseconds)==0;
            BOOST_INTERLOCKED_DECREMENT(&waiting_count);
            if(notified && !BOOST_INTERLOCKED_DECREMENT(&notify_count))
            {
                state_change_gate.unlock();
            }
            m.lock();
            return notified;
        }
        

    public:
        condition():
            notification_sem(BOOST_CREATE_SEMAPHORE(0,0,LONG_MAX,0)),
            waiting_count(0),
            notify_count(0)
        {
            state_change_gate.initialize();
        };
        

        ~condition()
        {
            BOOST_CLOSE_HANDLE(notification_sem);
        }

        template<typename scoped_lock_type>
        void wait(scoped_lock_type& m)
        {
            do_wait(m);
        }

        template<typename scoped_lock_type,typename predicate_type>
        void wait(scoped_lock_type& m,predicate_type pred)
        {
            while(!pred()) do_wait(m);
        }

        template<typename scoped_lock_type>
        bool timed_wait(scoped_lock_type& m,const xtime& xt)
        {
            return do_wait(m,detail::get_milliseconds_until_time(xt));
        }

        template<typename scoped_lock_type,typename predicate_type>
        bool timed_wait(scoped_lock_type& m,const xtime& xt,predicate_type pred)
        {
            while (!pred()) 
            {
                if (!timed_wait(m, xt)) return false;
            }
            return true;
        }


        void notify_one()
        {
            release_notification_sem(false);
        }
        
        void notify_all()
        {
            release_notification_sem(true);
        }
    };

}



#endif
