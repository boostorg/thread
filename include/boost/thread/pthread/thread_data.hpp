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
    class thread_interrupted
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
            bool interrupt_enabled;
            bool interrupt_requested;
            pthread_cond_t* current_cond;

            thread_data_base():
                done(false),join_started(false),joined(false),
                thread_exit_callbacks(0),
                interrupt_enabled(true),
                interrupt_requested(false),
                current_cond(0)
            {}
            virtual ~thread_data_base()
            {}

            virtual void run()=0;
        };

        BOOST_THREAD_DECL thread_data_base* get_current_thread_data();

        class interrupt_wrapper
        {
            thread_data_base* const thread_info;

            void check_for_interruption()
            {
                if(thread_info->interrupt_requested)
                {
                    thread_info->interrupt_requested=false;
                    throw thread_interrupted();
                }
            }
            
        public:
            explicit interrupt_wrapper(pthread_cond_t* cond):
                thread_info(detail::get_current_thread_data())
            {
                if(thread_info && thread_info->interrupt_enabled)
                {
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    check_for_interruption();
                    thread_info->current_cond=cond;
                }
            }
            ~interrupt_wrapper()
            {
                if(thread_info && thread_info->interrupt_enabled)
                {
                    lock_guard<mutex> guard(thread_info->data_mutex);
                    thread_info->current_cond=NULL;
                    check_for_interruption();
                }
            }
        };
    }
}


#endif
