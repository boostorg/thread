// Copyright (C) 2013-2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple user scheduler.
// 2013/11 Vicente J. Botet Escriba
//    rename loop_executor.

#ifndef BOOST_THREAD_EXECUTORS_LOOP_EXECUTOR_HPP
#define BOOST_THREAD_EXECUTORS_LOOP_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{

  class loop_executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:

    struct shared_state {
      typedef  executors::work work;
      /// the thread safe work queue
      concurrent::sync_queue<work > work_queue;

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

      /// loop_executor is not copyable.
      BOOST_THREAD_NO_COPYABLE(shared_state)

      /**
       * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
       *
       * \b Throws: Whatever exception is thrown while initializing the needed resources.
       */
      shared_state()
      {
      }
      /**
       * \b Effects: Destroys the thread pool.
       *
       * \b Synchronization: The completion of all the closures happen before the completion of the \c loop_executor destructor.
       */
      ~shared_state()
      {
        // signal to all the worker thread that there will be no more submissions.
        close();
      }

      /**
       * The main loop of the worker thread
       */
      void loop()
      {
        while (!closed())
        {
          schedule_one_or_yield();
        }
        while (try_executing_one())
        {
        }
      }

      /**
       * \b Effects: close the \c loop_executor for submissions.
       * The loop will work until there is no more closures to run.
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
       * If invoked closure throws an exception the \c loop_executor will call \c std::terminate, as is the case with threads.
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

      /**
       * run queued closures
       */
      void run_queued_closures()
      {
        sync_queue<work>::underlying_queue_type q = work_queue.underlying_queue();
        while (! q.empty())
        {
          work& task = q.front();
          task();
          q.pop_front();
        }
      }

    };
  public:
    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    loop_executor()
    : pimpl(make_shared<shared_state>())
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c loop_executor destructor.
     */
    ~loop_executor()
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
//    /**
//     * Effects: schedule one task or yields
//     * Throws: whatever the current task constructor throws or the task() throws.
//     */
//    void schedule_one_or_yield()
//    {
//      return pimpl->schedule_one_or_yield();
//    }


    /**
     * The main loop of the worker thread
     */
    void loop()
    {
      pimpl->loop();
    }

    /**
     * \b Effects: close the \c loop_executor for submissions.
     * The loop will work until there is no more closures to run.
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
     * If invoked closure throws an exception the \c loop_executor will call \c std::terminate, as is the case with threads.
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

    /**
     * run queued closures
     */
    void run_queued_closures()
    {
      pimpl->run_queued_closures();
    }
  private:
    shared_ptr<shared_state> pimpl;
  };
}
using executors::loop_executor;

}

#include <boost/config/abi_suffix.hpp>

#endif
