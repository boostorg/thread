// Copyright (C) 2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

// Derived loosely from work queue manager in "Programming POSIX Threads"
//   by David Butenhof.

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread_pool.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/bind.hpp>
#include <list>
#include <queue>
#include <stdexcept>
#include <cassert>

namespace boost {

class thread_pool::impl
{
public:
    impl(int max_threads, int min_threads, int timeout_secs,
        int timeout_nsecs);
    ~impl();

    void add(const boost::function0<void> &job);
    void join();
    void cancel();
    void detach();
    void worker_harness();

private:
    typedef enum
    {
        RUNNING,
        CANCELLING,
        JOINING,
        JOINED,
        DETACHED
    } thread_pool_state;

    typedef std::queue<boost::function0<void> > job_q;

    condition m_more_work;
    condition m_done;
    mutex m_prot;
    job_q m_jobs;
    thread_group m_workers;
    thread_pool_state   m_state;
    int m_max_threads;      // Max threads allowed
    int m_min_threads;
    int m_thread_count;     // Current number of threads
    int m_idle_count;       // Number of idle threads
    int m_timeout_secs;     // How long to keep idle threads
    int m_timeout_nsecs;
};

thread_pool::impl::impl(int max_threads, int min_threads, int timeout_secs,
    int timeout_nsecs)
    : m_state(RUNNING), m_max_threads(max_threads), m_min_threads(min_threads),
      m_thread_count(0), m_idle_count(0), m_timeout_secs(timeout_secs),
      m_timeout_nsecs(timeout_nsecs)
{
    // Immediately launch some worker threads.
    //
    // Not an exception safe implementation, yet.
    while (min_threads-- > 0)
    {
        m_workers.create_thread(
            bind(&thread_pool::impl::worker_harness, this));
        m_thread_count++;
    }
}

thread_pool::impl::~impl()
{
    // Join in the destructor, unless they have already
    //    joined or detached.
    mutex::scoped_lock lock(m_prot);
    if (m_state == RUNNING)
    {
        lock.unlock();
        join();
    }
}

void thread_pool::impl::add(const boost::function0<void> &job)
{
    mutex::scoped_lock lock(m_prot);

    // Note - can never reach this point if m_state == CANCELLED
    //  because the m_prot is held during the entire cancel operation.

    assert(m_state == RUNNING);

    m_jobs.push(job);
    if (m_idle_count > 0)
        m_more_work.notify_one();
    else if (m_thread_count < m_max_threads)
    {
        // No idle threads, and we're below our limit.  Spawn a new
        //  worker.

        // What we really need is thread::detach(), or "create suspended"
        m_workers.create_thread(
            bind(&thread_pool::impl::worker_harness, this));
        m_thread_count++;
    }
}

void thread_pool::impl::join()
{
    mutex::scoped_lock lock(m_prot);

    assert(m_state == RUNNING);

    if (m_thread_count > 0)
    {
        m_state = JOINING;

        // if any threads are idling, wake them.
        if (m_idle_count > 0)
            m_more_work.notify_all();

        // Track the shutdown progress of the threads.
        while (m_thread_count > 0)
            m_done.wait(lock);
    }

    m_workers.join_all();
    m_state = JOINED;
}

// This is a "weak" form of cancel which empties out the job queue and takes
//   the thread count down to zero.
//
// Upon receiving more work, the thread count would grow back up to
//   min_threads.
//
// Cancel will be much stronger once full thread cancellation is in place!

void thread_pool::impl::cancel()
{
    mutex::scoped_lock lock(m_prot);

    assert(m_state == RUNNING);

    if (m_thread_count > 0)
    {
        m_state = CANCELLING;

        // Cancelling kills any unexecuted jobs.
        while (!m_jobs.empty())
            m_jobs.pop();

        /* If we had cancel, this would be something like....
           m_workers.cancel_all();
           while(m_cancel_count > 0)
           m_all_cancelled.wait(lock);
        */
    }

    m_state = RUNNING;          // Go back to accepting work.
}

void thread_pool::impl::detach()
{
    mutex::scoped_lock lock(m_prot);
    if (m_state == RUNNING)
    {
        m_min_threads = 0;
        m_state = DETACHED;
    }
    else
    {
        // detach during/after a join has no effect - the join will
        //   complete.
    }
}

void thread_pool::impl::worker_harness()
{
    boost::thread me;

    xtime   timeout;
    int     timedout;

    mutex::scoped_lock lock(m_prot);

    for (;;)
    {
        timedout = 0;

        xtime_get(&timeout, boost::TIME_UTC);
        timeout.sec += m_timeout_secs;
        timeout.nsec += m_timeout_nsecs;

        while (m_jobs.empty() && (m_state == RUNNING))
        {
            m_idle_count++;
            bool status = m_more_work.timed_wait(lock, timeout);
            m_idle_count--;
            if (!status)
            {
                timedout = 1;
                return;
            }
        }

        if (!m_jobs.empty() && m_state != CANCELLING)
        {
            boost::function0<void> jobfunc = m_jobs.front();
            m_jobs.pop();
            lock.unlock();
            jobfunc();
            lock.lock();
        }
        else if (m_jobs.empty() && m_state == JOINING)
        {
            m_thread_count--;

            // If we are the last worker exiting, let everyone know about it!
            if (m_thread_count == 0)
                m_done.notify_all();

            return;
        }
        else if (m_jobs.empty() && m_state == DETACHED)
        {
            m_thread_count--;

            // If we are the last worker exiting, let everyone know about it!
            if (m_thread_count == 0)
            {
                lock.unlock();
                delete this;
            }

            return;
        }

        /*
         * If there's no more work, and we wait for as long as
         * we're allowed, then terminate this server thread.
         */
        if (m_jobs.empty() && timedout)
        {
            if (m_thread_count > m_min_threads)
            {
                m_thread_count--;

                if (m_state == DETACHED &&
                    m_thread_count == 0)
                {
                    lock.unlock();
                    delete this;
                    return;
                }

                // We aren't in a JOINING or CANCELLING state, so trim
                //   down our resource usage and clean ourselves up.
//              thread* thrd = m_workers.find(me);
                m_workers.remove_thread(me);
//              delete thrd;
                return;
            }
        }
    }
}

thread_pool::thread_pool(int max_threads, int min_threads, int timeout_secs,
    int timeout_nsecs)
    : m_pimpl(new impl(max_threads, min_threads, timeout_secs, timeout_nsecs))
{
}

thread_pool::~thread_pool()
{
    if (m_pimpl != NULL)
        delete m_pimpl;
}

void thread_pool::add(const boost::function0<void> &job)
{
    assert(m_pimpl);
    m_pimpl->add(job);
}

void thread_pool::join()
{
    assert(m_pimpl);
    m_pimpl->join();
}

void thread_pool::cancel()
{
    assert(m_pimpl);
    m_pimpl->cancel();
}

void thread_pool::detach()
{
    assert(m_pimpl);

    // Tell our implementation it is running detached.
    m_pimpl->detach();
    m_pimpl = NULL;
}

}   // namespace boost
