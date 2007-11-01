#ifndef BOOST_THREAD_THREAD_WIN32_HPP
#define BOOST_THREAD_THREAD_WIN32_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <exception>
#include <boost/thread/exceptions.hpp>
#include <ostream>
#include <boost/thread/detail/move.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread_time.hpp>
#include "thread_primitives.hpp"
#include "thread_heap_alloc.hpp"
#include <boost/utility.hpp>
#include <list>
#include <algorithm>
#include <boost/ref.hpp>

namespace boost
{
    class thread_cancelled
    {};

    namespace detail
    {
        struct thread_exit_callback_node;
        
        struct thread_data_base
        {
            long count;
            detail::win32::handle_manager thread_handle;
            detail::win32::handle_manager cancel_handle;
            boost::detail::thread_exit_callback_node* thread_exit_callbacks;
            bool cancel_enabled;
            unsigned id;

            thread_data_base():
                count(0),thread_handle(detail::win32::invalid_handle_value),
                cancel_handle(create_anonymous_event(detail::win32::manual_reset_event,detail::win32::event_initially_reset)),
                thread_exit_callbacks(0),
                cancel_enabled(true),
                id(0)
            {}
            virtual ~thread_data_base()
            {}

            friend void intrusive_ptr_add_ref(thread_data_base * p)
            {
                BOOST_INTERLOCKED_INCREMENT(&p->count);
            }
            
            friend void intrusive_ptr_release(thread_data_base * p)
            {
                if(!BOOST_INTERLOCKED_DECREMENT(&p->count))
                {
                    detail::heap_delete(p);
                }
            }

            virtual void run()=0;
        };
    }

    class BOOST_THREAD_DECL thread
    {
    private:
        thread(thread&);
        thread& operator=(thread&);

        void release_handle();

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
        boost::intrusive_ptr<detail::thread_data_base> thread_info;

        static unsigned __stdcall thread_start_function(void* param);

        void start_thread();
        
        explicit thread(boost::intrusive_ptr<detail::thread_data_base> data);

        boost::intrusive_ptr<detail::thread_data_base> get_thread_info() const;
    public:
        thread();
        ~thread();

        template <class F>
        explicit thread(F f):
            thread_info(detail::heap_new<thread_data<F> >(f))
        {
            start_thread();
        }
        template <class F>
        explicit thread(boost::move_t<F> f):
            thread_info(detail::heap_new<thread_data<F> >(f))
        {
            start_thread();
        }

        thread(boost::move_t<thread> x);
        thread& operator=(boost::move_t<thread> x);
        operator boost::move_t<thread>();
        boost::move_t<thread> move();

        void swap(thread& x);

        class id;
        id get_id() const;


        bool joinable() const;
        void join();
        bool timed_join(const system_time& wait_until);

        template<typename TimeDuration>
        inline bool timed_join(TimeDuration const& rel_time)
        {
            return timed_join(get_system_time()+rel_time);
        }
        void detach();

        static unsigned hardware_concurrency();

        typedef detail::win32::handle native_handle_type;
        native_handle_type native_handle();

        // backwards compatibility
        bool operator==(const thread& other) const;
        bool operator!=(const thread& other) const;

        static void yield();
        static void sleep(const system_time& xt);

        // extensions
        class cancel_handle;
        cancel_handle get_cancel_handle() const;
        void cancel();
        bool cancellation_requested() const;

        static thread self();
    };

    template<typename F>
    struct thread::thread_data<boost::reference_wrapper<F> >:
        detail::thread_data_base
    {
        F& f;
        
        thread_data(boost::reference_wrapper<F> f_):
            f(f_)
        {}
        
        void run()
        {
            f();
        }
    };
    

    namespace this_thread
    {
        class BOOST_THREAD_DECL disable_cancellation
        {
            disable_cancellation(const disable_cancellation&);
            disable_cancellation& operator=(const disable_cancellation&);
            
            bool cancel_was_enabled;
            friend class restore_cancellation;
        public:
            disable_cancellation();
            ~disable_cancellation();
        };

        class BOOST_THREAD_DECL restore_cancellation
        {
            restore_cancellation(const restore_cancellation&);
            restore_cancellation& operator=(const restore_cancellation&);
        public:
            explicit restore_cancellation(disable_cancellation& d);
            ~restore_cancellation();
        };

        thread::id BOOST_THREAD_DECL get_id();

        bool BOOST_THREAD_DECL cancellable_wait(detail::win32::handle handle_to_wait_for,unsigned long milliseconds);
        inline bool cancellable_wait(unsigned long milliseconds)
        {
            return cancellable_wait(detail::win32::invalid_handle_value,milliseconds);
        }

        void BOOST_THREAD_DECL cancellation_point();
        bool BOOST_THREAD_DECL cancellation_enabled();
        bool BOOST_THREAD_DECL cancellation_requested();
        thread::cancel_handle BOOST_THREAD_DECL get_cancel_handle();

        void BOOST_THREAD_DECL yield();
        template<typename TimeDuration>
        void sleep(TimeDuration const& rel_time)
        {
            cancellable_wait(static_cast<unsigned long>(rel_time.total_milliseconds()));
        }
    }

    class thread::id
    {
    private:
        unsigned thread_id;
            
        id(unsigned thread_id_):
            thread_id(thread_id_)
        {}
        friend class thread;
        friend id this_thread::get_id();
    public:
        id():
            thread_id(0)
        {}
            
        bool operator==(const id& y) const
        {
            return thread_id==y.thread_id;
        }
        
        bool operator!=(const id& y) const
        {
            return thread_id!=y.thread_id;
        }
        
        bool operator<(const id& y) const
        {
            return thread_id<y.thread_id;
        }
        
        bool operator>(const id& y) const
        {
            return thread_id>y.thread_id;
        }
        
        bool operator<=(const id& y) const
        {
            return thread_id<=y.thread_id;
        }
        
        bool operator>=(const id& y) const
        {
            return thread_id>=y.thread_id;
        }

        template<class charT, class traits>
        friend std::basic_ostream<charT, traits>& 
        operator<<(std::basic_ostream<charT, traits>& os, const id& x)
        {
            return os<<x.thread_id;
        }
    };

    inline bool thread::operator==(const thread& other) const
    {
        return get_id()==other.get_id();
    }
    
    inline bool thread::operator!=(const thread& other) const
    {
        return get_id()!=other.get_id();
    }

    class thread::cancel_handle
    {
    private:
        boost::detail::win32::handle_manager handle;
        friend class thread;
        friend cancel_handle this_thread::get_cancel_handle();

        cancel_handle(detail::win32::handle h_):
            handle(h_)
        {}
    public:
        cancel_handle(cancel_handle const& other):
            handle(other.handle.duplicate())
        {}
        cancel_handle():
            handle(0)
        {}

        void swap(cancel_handle& other)
        {
            handle.swap(other.handle);
        }
        
        cancel_handle& operator=(cancel_handle const& other)
        {
            cancel_handle temp(other);
            swap(temp);
            return *this;
        }

        void reset()
        {
            handle=0;
        }

        void cancel()
        {
            if(handle)
            {
                detail::win32::SetEvent(handle);
            }
        }

        typedef void(cancel_handle::*bool_type)();
        operator bool_type() const
        {
            return handle?&cancel_handle::cancel:0;
        }
    };
        
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
            detail::thread_exit_function_base* const thread_exit_func=detail::heap_new<detail::thread_exit_function<F> >(f);
            detail::add_thread_exit_function(thread_exit_func);
        }
    }

    class thread_group:
        private noncopyable
    {
    public:
        ~thread_group()
        {
            for(std::list<thread*>::iterator it=threads.begin(),end=threads.end();
                it!=end;
                ++it)
            {
                delete *it;
            }
        }

        template<typename F>
        thread* create_thread(F threadfunc)
        {
            boost::lock_guard<mutex> guard(m);
            thread* const new_thread=new thread(threadfunc);
            threads.push_back(new_thread);
            return new_thread;
        }
        
        void add_thread(thread* thrd)
        {
            if(thrd)
            {
                boost::lock_guard<mutex> guard(m);
                threads.push_back(thrd);
            }
        }
            
        void remove_thread(thread* thrd)
        {
            boost::lock_guard<mutex> guard(m);
            std::list<thread*>::iterator const it=std::find(threads.begin(),threads.end(),thrd);
            if(it!=threads.end())
            {
                threads.erase(it);
            }
        }
        
        void join_all()
        {
            boost::lock_guard<mutex> guard(m);
            
            for(std::list<thread*>::iterator it=threads.begin(),end=threads.end();
                it!=end;
                ++it)
            {
                (*it)->join();
            }
        }
        
        int size() const
        {
            boost::lock_guard<mutex> guard(m);
            return threads.size();
        }
        
    private:
        std::list<thread*> threads;
        mutable mutex m;
    };
}

#endif
