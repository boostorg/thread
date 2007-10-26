#ifndef BOOST_THREAD_THREAD_PTHREAD_HPP
#define BOOST_THREAD_THREAD_PTHREAD_HPP
// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <list>
#include <memory>

#include <pthread.h>
#include <boost/optional.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/shared_ptr.hpp>

namespace boost
{

    class thread;

    namespace detail
    {
        class thread_id;
    }
    
    namespace this_thread
    {
        detail::thread_id get_id();
    }
    
    namespace detail
    {
        class thread_id
        {
            boost::optional<pthread_t> id;

            friend class boost::thread;

            friend thread_id this_thread::get_id();

            thread_id(pthread_t id_):
                id(id_)
            {}
        
        public:
            thread_id()
            {}

            bool operator==(const thread_id& y) const
            {
                return (id && y.id) && (pthread_equal(*id,*y.id)!=0);
            }
        
            bool operator!=(const thread_id& y) const
            {
                return !(*this==y);
            }

            template<class charT, class traits>
            friend std::basic_ostream<charT, traits>& 
            operator<<(std::basic_ostream<charT, traits>& os, const thread_id& x)
            {
                if(x.id)
                {
                    return os<<*x.id;
                }
                else
                {
                    return os<<"{Not-any-thread}";
                }
            }
        
        };
    }

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
            bool done;
            bool join_started;
            bool joined;
            boost::detail::thread_exit_callback_node* thread_exit_callbacks;
            bool cancel_enabled;
            bool cancel_requested;

            thread_data_base():
                done(false),join_started(false),joined(false),
                thread_exit_callbacks(0),
                cancel_enabled(true),
                cancel_requested(false)
            {}
            virtual ~thread_data_base()
            {}

            virtual void run()=0;
        };
    }

    struct xtime;
    class BOOST_THREAD_DECL thread
    {
    private:
        thread(thread&);
        thread& operator=(thread&);

        template<typename F>
        struct thread_data:
            detail::thread_data_base
        {
            F f;

            thread_data(F f_):
                f(f_)
            {}
            thread_data(boost::move_t<F> f_):
                f(f_)
            {}
            
            void run()
            {
                f();
            }
        };
        
        mutable boost::mutex thread_info_mutex;
        boost::shared_ptr<detail::thread_data_base> thread_info;

        void start_thread();
        
        explicit thread(boost::shared_ptr<detail::thread_data_base> data);

        boost::shared_ptr<detail::thread_data_base> get_thread_info() const;
        
    public:
        thread();
        ~thread();

        template <class F>
        explicit thread(F f):
            thread_info(new thread_data<F>(f))
        {
            start_thread();
        }
        template <class F>
        thread(boost::move_t<F> f):
            thread_info(new thread_data<F>(f))
        {
            start_thread();
        }

        explicit thread(boost::move_t<thread> x);
        thread& operator=(boost::move_t<thread> x);
        operator boost::move_t<thread>();
        boost::move_t<thread> move();

        void swap(thread& x);

        typedef detail::thread_id id;
        
        id get_id() const;

        bool joinable() const;
        void join();
        void detach();

        static unsigned hardware_concurrency();

        // backwards compatibility
        bool operator==(const thread& other) const;
        bool operator!=(const thread& other) const;

        static void sleep(const system_time& xt);
        static void yield();

        // extensions
        class cancel_handle;
        cancel_handle get_cancel_handle() const;
        void cancel();
        bool cancellation_requested() const;
    };

    namespace this_thread
    {
        class disable_cancellation
        {
            disable_cancellation(const disable_cancellation&);
            disable_cancellation& operator=(const disable_cancellation&);
            
            bool cancel_was_enabled;
            friend class restore_cancellation;
        public:
            disable_cancellation();
            ~disable_cancellation();
        };

        class restore_cancellation
        {
            restore_cancellation(const restore_cancellation&);
            restore_cancellation& operator=(const restore_cancellation&);
        public:
            explicit restore_cancellation(disable_cancellation& d);
            ~restore_cancellation();
        };

        inline thread::id get_id()
        {
            return thread::id(pthread_self());
        }

        void BOOST_THREAD_DECL cancellation_point();
        bool BOOST_THREAD_DECL cancellation_enabled();
        bool BOOST_THREAD_DECL cancellation_requested();

        inline void yield()
        {
            thread::yield();
        }
        
        template<typename TimeDuration>
        inline void sleep(TimeDuration const& rel_time)
        {
            thread::sleep(get_system_time()+rel_time);
        }
    }

    namespace detail
    {
        struct thread_exit_function_base
        {
            virtual ~thread_exit_function_base()
            {}
            virtual void operator()() const=0;
        };
        
        template<typename F>
        struct thread_exit_function:
            thread_exit_function_base
        {
            F f;
            
            thread_exit_function(F f_):
                f(f_)
            {}
            
            void operator()() const
            {
                f();
            }
        };
        
        void add_thread_exit_function(thread_exit_function_base*);
    }
    
    namespace this_thread
    {
        template<typename F>
        void at_thread_exit(F f)
        {
            detail::thread_exit_function_base* const thread_exit_func=new detail::thread_exit_function<F>(f);
            detail::add_thread_exit_function(thread_exit_func);
        }
    }

    class BOOST_THREAD_DECL thread_group : private noncopyable
    {
    public:
        thread_group();
        ~thread_group();

        thread* create_thread(const function0<void>& threadfunc);
        void add_thread(thread* thrd);
        void remove_thread(thread* thrd);
        void join_all();
        int size() const;

    private:
        std::list<thread*> m_threads;
        mutex m_mutex;
    };
} // namespace boost


#endif
