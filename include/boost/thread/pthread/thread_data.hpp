#ifndef BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
#define BOOST_THREAD_PTHREAD_THREAD_DATA_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <boost/thread/detail/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/optional.hpp>
#include <pthread.h>
#include "condition_variable_fwd.hpp"

namespace boost
{
    class thread_cancelled
    {};

    namespace detail
    {
        struct thread_exit_callback_node;
        
        struct thread_data_base
        {
            boost::shared_ptr<thread_data_base> self;
            pthread_t thread_handle;
            boost::mutex data_mutex;
            boost::condition_variable done_condition;
            boost::mutex sleep_mutex;
            boost::condition_variable sleep_condition;
            bool done;
            bool join_started;
            bool joined;
            boost::detail::thread_exit_callback_node* thread_exit_callbacks;
            bool cancel_enabled;
            bool cancel_requested;
            pthread_cond_t* current_cond;

            thread_data_base():
                done(false),join_started(false),joined(false),
                thread_exit_callbacks(0),
                cancel_enabled(true),
                cancel_requested(false),
                current_cond(0)
            {}
            virtual ~thread_data_base()
            {}

            virtual void run()=0;
        };

        BOOST_THREAD_DECL thread_data_base* get_current_thread_data();

        class cancel_wrapper
        {
            thread_data_base* const thread_info;

            void check_cancel()
            {
                if(thread_info->cancel_requested)
                {
                    thread_info->cancel_requested=false;
                    throw thread_cancelled();
                }
            }
            
        public:
            explicit cancel_wrapper(pthread_cond_t* cond):
                thread_info(detail::get_current_thread_data())
            {
                if(thread_info && thread_info->cancel_enabled)
                {
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    check_cancel();
                    thread_info->current_cond=cond;
                }
            }
            ~cancel_wrapper()
            {
                if(thread_info && thread_info->cancel_enabled)
                {
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    thread_info->current_cond=NULL;
                    check_cancel();
                }
            }
        };
    }
}


#endif
