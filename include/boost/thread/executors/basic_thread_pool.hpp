// Copyright (C) 2013-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/09 Vicente J. Botet Escriba
//    Adapt to boost from CCIA C++11 implementation
//    first implementation of a simple pool thread using a vector of threads and a sync_queue.

#ifndef BOOST_THREAD_EXECUTORS_BASIC_THREAD_POOL_HPP
#define BOOST_THREAD_EXECUTORS_BASIC_THREAD_POOL_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/csbl/vector.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  class basic_thread_pool
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:

    struct shared_state {
      typedef  executors::work work;
      /// the kind of stored threads are scoped threads to ensure that the threads are joined.
      /// A move aware vector type
      typedef scoped_thread<> thread_t;
      typedef csbl::vector<thread_t> thread_vector;

      /// the thread safe work queue
      concurrent::sync_queue<work > work_queue;
      /// A move aware vector
      thread_vector threads;

    public:
      /**
       * Effects: try to execute one task.
       * Returns: whether a task has been executed.
       * Throws: whatever the current task constructor throws or the task() throws.
       */
      bool try_executing_one()
      {
        try
        {
          work task;
          if (work_queue.try_pull(task) == queue_op_status::success)
          {
            task();
            return true;
          }
          return false;
        }
        catch (...)
        {
          std::terminate();
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
    private:

      /**
       * The main loop of the worker threads
       */
      void worker_thread()
      {
        try
        {
          for(;;)
          {
            work task;
            queue_op_status st = work_queue.wait_pull(task);
            if (st == queue_op_status::closed) return;
            task();
          }
        }
        catch (...)
        {
          std::terminate();
          return;
        }
      }
  #if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <class AtThreadEntry>
      void worker_thread1(AtThreadEntry& at_thread_entry)
      {
        at_thread_entry();
        worker_thread();
      }
  #endif
      void worker_thread2(void(*at_thread_entry)())
      {
        at_thread_entry();
        worker_thread();
      }
      template <class AtThreadEntry>
      void worker_thread3(BOOST_THREAD_FWD_REF(AtThreadEntry) at_thread_entry)
      {
        at_thread_entry();
        worker_thread();
      }
      static void do_nothing_at_thread_entry() {}

    public:
      /// basic_thread_pool is not copyable.
      BOOST_THREAD_NO_COPYABLE(shared_state)

      /**
       * \b Effects: creates a thread pool that runs closures on \c thread_count threads.
       *
       * \b Throws: Whatever exception is thrown while initializing the needed resources.
       */
      shared_state(unsigned const thread_count = thread::hardware_concurrency()+1)
      {
        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
  #if 1
            thread th (&shared_state::worker_thread, this);
            threads.push_back(thread_t(boost::move(th)));
  #else
            threads.push_back(thread_t(&shared_state::worker_thread, this)); // do not compile
  #endif
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }
      /**
       * \b Effects: creates a thread pool that runs closures on \c thread_count threads
       * and executes the at_thread_entry function at the entry of each created thread. .
       *
       * \b Throws: Whatever exception is thrown while initializing the needed resources.
       */
  #if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <class AtThreadEntry>
      shared_state( unsigned const thread_count, AtThreadEntry& at_thread_entry)
      {
        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
            thread th (&shared_state::worker_thread1<AtThreadEntry>, this, at_thread_entry);
            threads.push_back(thread_t(boost::move(th)));
            //threads.push_back(thread_t(&shared_state::worker_thread, this)); // do not compile
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }
  #endif
      shared_state( unsigned const thread_count, void(*at_thread_entry)())
      {
        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
            thread th (&shared_state::worker_thread2, this, at_thread_entry);
            threads.push_back(thread_t(boost::move(th)));
            //threads.push_back(thread_t(&shared_state::worker_thread, this)); // do not compile
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }
      template <class AtThreadEntry>
      shared_state( unsigned const thread_count, BOOST_THREAD_FWD_REF(AtThreadEntry) at_thread_entry)
      {
        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
            thread th (&shared_state::worker_thread3<AtThreadEntry>, this, boost::forward<AtThreadEntry>(at_thread_entry));
            threads.push_back(thread_t(boost::move(th)));
            //threads.push_back(thread_t(&shared_state::worker_thread, this)); // do not compile
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }
      /**
       * \b Effects: Destroys the thread pool.
       *
       * \b Synchronization: The completion of all the closures happen before the completion of the \c basic_thread_pool destructor.
       */
      ~shared_state()
      {
        // signal to all the worker threads that there will be no more submissions.
        close();
        // joins all the threads as the threads were scoped_threads
      }

      /**
       * \b Effects: join all the threads.
       */
      void join()
      {
        for (unsigned i = 0; i < threads.size(); ++i)
        {
          threads[i].join();
        }
      }

      /**
       * \b Effects: close the \c basic_thread_pool for submissions.
       * The worker threads will work until there is no more closures to run.
       */
      void close()
      {
        work_queue.close();
      }

      /**
       * \b Returns: whether the pool is closed for submissions.
       */
      bool closed()
      {
        return work_queue.closed();
      }

      /**
       * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
       *
       * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
       * If invoked closure throws an exception the \c basic_thread_pool will call \c std::terminate, as is the case with threads.
       *
       * \b Synchronization: completion of \c closure on a particular thread happens before destruction of thread's thread local variables.
       *
       * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
       * Whatever exception that can be throw while storing the closure.
       */

  #if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename Closure>
      void submit(Closure & closure)
      {
        work_queue.push(work(closure));
      }
  #endif
      void submit(void (*closure)())
      {
        work_queue.push(work(closure));
      }

      template <typename Closure>
      void submit(BOOST_THREAD_RV_REF(Closure) closure)
      {
        work_queue.push(work(boost::forward<Closure>(closure)));
      }

      /**
       * \b Requires: This must be called from an scheduled task.
       *
       * \b Effects: reschedule functions until pred()
       */
      template <typename Pred>
      bool reschedule_until(Pred const& pred)
      {
        do {
          if ( ! try_executing_one())
          {
            return false;
          }
        } while (! pred());
        return true;
      }
    };

  public:
    /**
     * \b Effects: creates a thread pool that runs closures on \c thread_count threads.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    basic_thread_pool(unsigned const thread_count = thread::hardware_concurrency()+1)
    : pimpl(make_shared<shared_state>(thread_count))
    {
    }

    /**
     * \b Effects: creates a thread pool that runs closures on \c thread_count threads
     * and executes the at_thread_entry function at the entry of each created thread. .
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <class AtThreadEntry>
    basic_thread_pool( unsigned const thread_count, AtThreadEntry& at_thread_entry)
    : pimpl(make_shared<shared_state>(thread_count, at_thread_entry))
    {
    }
#endif

    basic_thread_pool( unsigned const thread_count, void(*at_thread_entry)())
    : pimpl(make_shared<shared_state>(thread_count, at_thread_entry))
    {
    }
    template <class AtThreadEntry>
    basic_thread_pool( unsigned const thread_count, BOOST_THREAD_FWD_REF(AtThreadEntry) at_thread_entry)
    : pimpl(make_shared<shared_state>(thread_count, at_thread_entry))
    {
    }

    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c basic_thread_pool destructor.
     */
    ~basic_thread_pool()
    {
    }

    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one()
    {
      return pimpl->try_executing_one();
    }

    /**
     * \b Effects: join all the threads.
     */
    void join()
    {
      pimpl->join();
    }

    /**
     * \b Effects: close the \c basic_thread_pool for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close()
    {
      pimpl->close();
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
      return pimpl->closed();
    }

    /**
     * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
     *
     * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the \c basic_thread_pool will call \c std::terminate, as is the case with threads.
     *
     * \b Synchronization: completion of \c closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      pimpl->submit(closure);
    }
#endif
    void submit(void (*closure)())
    {
      pimpl->submit(closure);
    }

    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      pimpl->submit(boost::forward<Closure>(closure));
    }

    /**
     * \b Requires: This must be called from an scheduled task.
     *
     * \b Effects: reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      return pimpl->reschedule_until(pred);
    }

    void schedule_one_or_yield()
    {
      return pimpl->schedule_one_or_yield();
    }

  private:
    shared_ptr<shared_state> pimpl;
  };
}
using executors::basic_thread_pool;

}

#include <boost/config/abi_suffix.hpp>

#endif
