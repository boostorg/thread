// Copyright (C) 2002 David Moore
//
// Based on Boost.Threads
// Copyright (C) 2001
// William E. Kempf
//
// Derived loosely from work queue manager in "Programming POSIX Threads"
//   by David Butenhof.
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.


#ifndef BOOST_THREAD_POOL_IMPL_JDM031802_HPP
#define BOOST_THREAD_POOL_IMPL_JDM031802_HPP


#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

#include <list>
#include <queue>

namespace boost {

    namespace detail { namespace thread {
        

        // detail class thread_grp
        //
        // Much like thread group, but delete_thread_equal_to
        //  uses operator== on thread to search by value
        //
        // Also doesn't require additional mutex since 
        //  outer thread_pool class will manage the mutual
        //  exclusion.
        //
        // Used to let a pool thread advise the pool that it should
        //   be removed from the set of join()able threads.

        class thread_grp
        {
        public:
            thread_grp();
            ~thread_grp();

            boost::thread* create_thread(const function0<void>& threadfunc);
            void delete_thread_equal_to(boost::thread *thrd);
            void join_all();
        private:
            std::list<boost::thread*> m_threads;
        };

        // thread_pool uses the pimpl idiom to allow for detach()ing
        //   the thread_pool object from its worker threads.
        //
        // Once detached, a thread_pool_impl will self-destruct once its
        //   m_thread_count reaches zero.
        //

        class thread_pool_impl
        {
        public:
            thread_pool_impl(int max_threads, int min_threads,int timeout_secs); 
            ~thread_pool_impl();

            void add(const boost::function0<void> &job);
            void join();
            void cancel();
            void detach();

        private:
            typedef enum
            {
                RUNNING,
                CANCELLING,
                JOINING,
                JOINED,
                DETACHED
            } thread_pool_state;
 
            static      void _worker_harness(void *arg);
            condition   m_more_work;
            condition   m_done;    
            mutex       m_prot;

            typedef std::queue<boost::function0<void> > job_q;
            job_q    m_jobs;

            detail::thread::thread_grp m_workers;

            thread_pool_state   m_state;
            int         m_max_threads;      // Max threads allowed
            int         m_min_threads;
            int         m_thread_count;     // Current number of threads
            int         m_idle_count;       // Number of idle threads

            int         m_timeout_secs;     // How long to keep idle threads
        };

    } // namespace thread
    } // namespace detail

} // namespace boost

#endif
