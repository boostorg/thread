// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_WEK070601_HPP
#define BOOST_THREAD_WEK070601_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <list>
#include <memory>

#include <pthread.h>
#include <boost/optional.hpp>

namespace boost {

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

    struct xtime;
    class BOOST_THREAD_DECL thread : private noncopyable
    {
    public:
        thread();
        explicit thread(const function0<void>& threadfunc);
        ~thread();

        bool operator==(const thread& other) const;
        bool operator!=(const thread& other) const;

        void join();

        static void sleep(const xtime& xt);
        static void yield();

        typedef detail::thread_id id;
        
        id get_id() const
        {
            return m_id;
        }

    private:
        id m_id;
        bool m_joinable;
    };

    namespace this_thread
    {
        inline thread::id get_id()
        {
            return thread::id(pthread_self());
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


#endif // BOOST_THREAD_WEK070601_HPP
