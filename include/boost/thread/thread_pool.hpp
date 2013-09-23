// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/09 Vicente J. Botet Escriba
//    Adapt to boost from CCIA C++11 implementation
//    first implementation of a simple pool thread using a vector of threads and a sync_queue.

#ifndef BOOST_THREAD_THREAD_POOL_HPP
#define BOOST_THREAD_THREAD_POOL_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/detail/function_wrapper.hpp>

#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
#include <boost/function.hpp>
#else
#include <functional>
#endif

#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
#include <boost/container/vector.hpp>
#else
#include <vector>
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  class thread_pool
  {
    /// type-erasure to store the works to do
    typedef  detail::function_wrapper work;
    /// the kind of stored threads are scoped threads to ensure that the threads are joined.
    /// A move aware vector type
    typedef scoped_thread<> thread_t;
#if defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    typedef container::vector<thread_t> thread_vector;
#else
    typedef std::vector<thread_t> thread_vector;
#endif

    /// the thread safe work queue
    sync_queue<work > work_queue;
    /// A move aware vector
    thread_vector threads;

    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one()
    {
      work task;
      try
      {
        if (work_queue.try_pull(task))
        {
          task();
          return true;
        }
        return false;
      }
      catch (std::exception& ex)
      {
        return false;
      }
      catch (...)
      {
        return false;
      }
    }
    /**
     * Effects: schedule one task or yields
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    void schedule_one_or_yield()
    {
        if ( ! try_executing_one())
        {
          this_thread::yield();
        }
    }
    /**
     * The main loop of the worker threads
     */
    void worker_thread()
    {
      while (!is_closed())
      {
        schedule_one_or_yield();
      }
    }

  public:
    /// thread_pool is not copyable.
    BOOST_THREAD_NO_COPYABLE(thread_pool)

    /**
     * Effects: creates a thread pool that runs closures on @c thread_count threads.
     */
    thread_pool(unsigned const thread_count = thread::hardware_concurrency())
    {
      try
      {
        for (unsigned i = 0; i < thread_count; ++i)
        {
          threads.push_back(thread_t(&thread_pool::worker_thread, this));
        }
      }
      catch (...)
      {
        close();
        throw;
      }
    }
    /**
     * Effects: Destroys the thread pool.
     * Synchronization: The completion of all the closures happen before the completion of the thread pool destructor.
     */
    ~thread_pool()
    {
      // signal to all the worker threads that there will be no more submissions.
      close();
      // joins all the threads as the threads were scoped_threads
    }

    /**
     * Effects: close the thread_pool for submissions. The worker threads will work until
     */
    void close()
    {
      work_queue.close();
    }

    /**
     * Returns: whether the pool is closed for submissions.
     */
    bool is_closed()
    {
      return work_queue.closed();
    }

    /**
     * Effects: The specified function will be scheduled for execution at some point in the future.
     * If invoking closure throws an exception the thread pool will call std::terminate, as is the case with threads.
     * Synchronization: completion of closure on a particular thread happens before destruction of thread's thread local variables.
     * Throws: sync_queue_is_closed if the thread pool is closed.
     *
     */
    template <typename Closure>
    void submit(Closure const& closure)
    {
      work w ((closure));
      work_queue.push(boost::move(w));
      //work_queue.push(work(closure));
    }
    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      work w =boost::move(closure);
      work_queue.push(boost::move(w));
      //work_queue.push(work(boost::move(closure)));
    }

    /**
     * This must be called from an scheduled task.
     * Effects: reschedule functions until pred()
     */
    template <typename Pred>
    void reschedule_until(Pred const& pred)
    {
      do {
        schedule_one_or_yield();
      } while (! pred());
    }

  };

}

#include <boost/config/abi_suffix.hpp>

#endif
