/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *    8 Feb 01  Initial version.
 *    1 Jun 01  Added boost::thread initial implementation.
 *    3 Jul 01  Redesigned boost::thread to be noncopyable.
 */
 
#ifndef BOOST_THREAD_HPP
#define BOOST_THREAD_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <stdexcept>
#include <list>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost
{
    class lock_error : public std::runtime_error
    {
    public:
        lock_error();
    };

    class thread : boost::noncopyable
    {
    public:
        thread();
        thread(const boost::function0<void>& threadfunc);
        ~thread();

        bool operator==(const thread& other);
        bool operator!=(const thread& other);

        void join();
        bool try_join();
        bool timed_join(const xtime& xt);

        static void sleep(const xtime& xt);
        static void yield();

    private:
#if defined(BOOST_HAS_WINTHREADS)
        unsigned long m_thread;
        unsigned int m_id;
#elif defined(BOOST_HAS_PTHREADS)
        class thread_list;
        friend class thread_list;

        pthread_t m_thread;
        mutex m_mutex;
        condition m_cond;
        bool m_alive;
        thread_list* m_list;
#endif
    };

    class thread_group : boost::noncopyable
    {
    public:
        thread_group();
        ~thread_group();

        thread* create_thread(const boost::function0<void>& threadfunc);
        void add_thread(thread* thrd);
        void remove_thread(thread* thrd);
        void join_all();

    private:
        std::list<thread*> m_threads;
        mutex m_mutex;
    };
} // namespace boost

#endif // BOOST_THREAD_HPP
