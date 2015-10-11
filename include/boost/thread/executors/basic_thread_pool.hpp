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
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/csbl/vector.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/function.hpp>

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

    struct shared_state : enable_shared_from_this<shared_state> {
      typedef  executors::work work;
      /// the kind of stored threads are scoped threads to ensure that the threads are joined.
      /// A move aware vector type
      //typedef scoped_thread<> thread_t;
      typedef thread thread_t;
      typedef csbl::vector<thread_t> thread_vector;

      /// the thread safe work queue
      concurrent::sync_queue<work > work_queue;
      /// A move aware vector
      thread_vector threads;
      unsigned const thread_count;
      boost::function<void(basic_thread_pool)> at_thread_entry;
      friend class basic_thread_pool;


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
        // fixme: this call results on segmentation fault
        //at_thread_entry(basic_thread_pool(this->shared_from_this()));
        try
        {
          for(;;)
          {
            work task;
            queue_op_status st = work_queue.wait_pull(task);
            if (st == queue_op_status::closed) break;
            task();
          }
        }
        catch (...)
        {
          std::terminate();
          return;
        }
      }

      static void do_nothing_at_thread_entry(basic_thread_pool) {}

      void init()
      {
        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
            thread th (&shared_state::worker_thread, this);
            threads.push_back(thread_t(boost::move(th)));
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }

    public:
      /// basic_thread_pool is not copyable.
      BOOST_THREAD_NO_COPYABLE(shared_state)

      /**
       * \b Effects: creates a thread pool that runs closures on \c thread_count threads.
       *
       * \b Throws: Whatever exception is thrown while initializing the needed resources.
       */
      shared_state(unsigned const thread_count = thread::hardware_concurrency()+1)
      : thread_count(thread_count),
        at_thread_entry(do_nothing_at_thread_entry)
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
      shared_state( unsigned const thread_count, AtThreadEntry& at_thread_entry)
      : thread_count(thread_count),
        at_thread_entry(at_thread_entry)
      {
      }
  #endif
      shared_state( unsigned const thread_count, void(*at_thread_entry)(basic_thread_pool))
      : thread_count(thread_count),
        at_thread_entry(at_thread_entry)
      {
      }
      template <class AtThreadEntry>
      shared_state( unsigned const thread_count, BOOST_THREAD_FWD_REF(AtThreadEntry) at_thread_entry)
      : thread_count(thread_count),
        at_thread_entry(boost::move(at_thread_entry))
      {
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
        join();
        // joins all the threads as the threads were scoped_threads
      }

      /**
       * \b Effects: join all the threads.
       */
      void join()
      {
        for (unsigned i = 0; i < threads.size(); ++i)
        {
          if (this_thread::get_id() == threads[i].get_id()) continue;
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

      void submit(BOOST_THREAD_RV_REF(work) closure)  {
        work_queue.push(boost::move(closure));
      }

  #if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
      template <typename Closure>
      void submit(Closure & closure)
      {
        //work_queue.push(work(closure));
        submit(work(closure));

      }
  #endif
      void submit(void (*closure)())
      {
        //work_queue.push(work(closure));
        submit(work(closure));
      }

      template <typename Closure>
      void submit(BOOST_THREAD_FWD_REF(Closure) closure)
      {
        //work_queue.push(work(boost::forward<Closure>(closure)));
        work w((boost::forward<Closure>(closure)));
        submit(boost::move(w));
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

    /**
     * \b Effects: creates a thread pool with this shared state.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    friend struct shared_state;
    basic_thread_pool(shared_ptr<shared_state> ptr)
    : pimpl(ptr)
    {
    }
  public:
    /**
     * \b Effects: creates a thread pool that runs closures on \c thread_count threads.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    basic_thread_pool(unsigned const thread_count = thread::hardware_concurrency()+1)
    : pimpl(make_shared<shared_state>(thread_count))
    {
      pimpl->init();
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
      pimpl->init();
    }
#endif

    basic_thread_pool( unsigned const thread_count, void(*at_thread_entry)(basic_thread_pool))
    : pimpl(make_shared<shared_state>(thread_count, at_thread_entry))
    {
      pimpl->init();
    }
    template <class AtThreadEntry>
    basic_thread_pool( unsigned const thread_count, BOOST_THREAD_FWD_REF(AtThreadEntry) at_thread_entry)
    : pimpl(make_shared<shared_state>(thread_count, boost::forward<AtThreadEntry>(at_thread_entry)))
    {
      pimpl->init();
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
//    void submit(BOOST_THREAD_RV_REF(work) closure)  {
//      work_queue.push(boost::move(closure));
//    }

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
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
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
