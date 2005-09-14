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
            long const waiters=BOOST_INTERLOCKED_READ(&waiting_count);
            if(waiters)
            {
                long const count_to_unlock=release_all?waiters:1;
                BOOST_INTERLOCKED_EXCHANGE(&notify_count,count_to_unlock);
                BOOST_RELEASE_SEMAPHORE(notification_sem,count_to_unlock,NULL);
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
        
        template<typename scoped_lock_type,typename predicate_type>
        bool do_wait(scoped_lock_type& m,predicate_type& pred,unsigned time_to_wait_in_milliseconds=BOOST_INFINITE)
        {
            {
                gate_scoped_lock lock(state_change_gate);
                if(pred())
                {
                    return true;
                }
                m.unlock();
                BOOST_INTERLOCKED_INCREMENT(&waiting_count);
            }
            
            bool const notified=BOOST_WAIT_FOR_SINGLE_OBJECT(notification_sem,time_to_wait_in_milliseconds)==0;
            BOOST_INTERLOCKED_DECREMENT(&waiting_count);
            if(notified && !BOOST_INTERLOCKED_DECREMENT(&notify_count))
            {
                state_change_gate.unlock();
            }
            m.lock();
            return notified && pred();
        }
        

    public:
        condition():
            notification_sem(BOOST_CREATE_SEMAPHORE(NULL,0,LONG_MAX,NULL)),
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
            once_predicate p;
            do_wait(m,p);
        }

        template<typename scoped_lock_type,typename predicate_type>
        void wait(scoped_lock_type& m,predicate_type pred)
        {
            if(pred())
            {
                return;
            }
            while(!do_wait(m,pred));
        }

        template<typename scoped_lock_type>
        bool timed_wait(scoped_lock_type& m,const xtime& xt)
        {
            once_predicate p;
            return do_wait(m,p,detail::get_milliseconds_until_time(xt));
        }

        template<typename scoped_lock_type,typename predicate_type>
        bool timed_wait(scoped_lock_type& m,const xtime& xt,predicate_type pred)
        {
            if(pred())
            {
                return true;
            }
            while(!do_wait(m,pred,detail::get_milliseconds_until_time(xt)))
            {
                if(!detail::get_milliseconds_until_time(xt))
                {
                    return false;
                }
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
