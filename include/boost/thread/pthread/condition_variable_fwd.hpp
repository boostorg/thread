#ifndef BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
#define BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <pthread.h>
#include <boost/thread/locks.hpp>
#include <boost/thread/thread_time.hpp>

namespace boost
{
    class condition_variable
    {
    private:
        pthread_cond_t cond;
        
        condition_variable(condition_variable&);
        condition_variable& operator=(condition_variable&);

        struct interruption_checker;
    public:
        condition_variable();
        ~condition_variable();

        void wait(unique_lock<mutex>& m);

        template<typename predicate_type>
        void wait(unique_lock<mutex>& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }

        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& wait_until);

        template<typename predicate_type>
        bool timed_wait(unique_lock<mutex>& m,boost::system_time const& wait_until,predicate_type pred)
        {
            while (!pred())
            {
                if(!timed_wait(m, wait_until))
                    return false;
            }
            return true;
        }

        void notify_one();
        void notify_all();
    };
}

#endif
