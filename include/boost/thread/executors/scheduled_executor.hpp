// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULED_EXECUTOR_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULED_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/queues/sync_timed_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/executor.hpp>
#include <boost/thread/executors/executor_wrapper.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/chrono/chrono.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{

  class scheduled_executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:
    typedef  executors::work_executor_delegate timed_work;
    typedef  scoped_thread<> thread_t;

    /// the thread safe work queue
    sync_timed_queue< work_executor_delegate > work_queue;
    executor& ex;
    chrono::nanoseconds granulatity;
    thread_t thr;

  public:
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
        // take an external lock
        if (work_queue.try_pull_front(task) == queue_op_status::success)
        {
          ex.submit(task);
          return true;
        }
        return false;
      }
      catch (std::exception& )
      {
        return false;
      }
      catch (...)
      {
        return false;
      }
    }
  private:
    /**
     * Effects: schedule one task or slleps for a while
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    void schedule_one_or_sleep()
    {
        if ( ! try_executing_one())
        {
          this_thread::sleep_for(granulatity);
        }
    }


    /**
     * The main loop of the worker thread
     */
    void worker_thread()
    {
      while (!closed())
      {
        schedule_one_or_sleep();
      }
      while ( ! work_queue.empty())
      {
        schedule_one_or_sleep();
      }
    }

  public:
    /// scheduled_executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(scheduled_executor)

    /**
     * \b Effects: creates a scheduled_executor that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    scheduled_executor(executor& ex, chrono::duration<Rep, Period> granularity=chrono::milliseconds(100))
    : ex(ex), granularity(granularity), thr(&scheduled_executor::worker_thread, this);
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c scheduled_executor destructor.
     */
    ~scheduled_executor()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c scheduled_executor for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
      ex.close();
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
     * If invoked closure throws an exception the \c scheduled_executor will call \c std::terminate, as is the case with threads.
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
      return ex.submit(closure);
    }
#endif
    void submit(void (*closure)())
    {
      return ex.submit(closure);
    }

    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      return ex.submit(closure);
    }

    template <class Clock, class Duration>
    void submit_at(chrono::time_point<Clock,Duration> abs_time, work&& closure);
    template <class Rep, class Period>
    void submit_after(chrono::duration<Rep,Period> rel_time, work&& closure);
    template <class Clock, class Duration, typename Closure>
    void submit_at(chrono::time_point<Clock,Duration> abs_time, Closure&& closure);
    template <class Rep, class Period, typename Closure>
    void submit_after(chrono::duration<Rep,Period> rel_time, Closure&& closure);

    /**
     * \b Requires: This must be called from an scheduled task.
     *
     * \b Effects: reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      do {
        if ( ! ex.try_executing_one())
          if ( ! try_executing_one())
            if ( work_queue.empty())
            {
              return false;
            } else {
              this_thread::sleep_for(granulatity);
            }
      } while (! pred());
      return true;
    }


  };
}
using executors::scheduled_executor;

}

#include <boost/config/abi_suffix.hpp>

#endif
