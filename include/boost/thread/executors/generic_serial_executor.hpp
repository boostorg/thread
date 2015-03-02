// Copyright (C) 2013,2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_GENERIC_SERIAL_EXECUTOR_HPP
#define BOOST_THREAD_GENERIC_SERIAL_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/decay.hpp>

#include <boost/thread/caller_context.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  class generic_serial_executor
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:

    struct shared_state {
      typedef  executors::work work;
      typedef  scoped_thread<> thread_t;

      /// the thread safe work queue
      concurrent::sync_queue<work > work_queue;
      generic_executor ex;
      thread_t thr;

      struct try_executing_one_task {
        work& task;
        boost::promise<void> &p;
        try_executing_one_task(work& task, boost::promise<void> &p)
        : task(task), p(p) {}
        void operator()() {
          try {
            task();
            p.set_value();
          } catch (...)
          {
            p.set_exception(current_exception());
          }
        }
      };
    public:
      /**
       * \par Returns
       * The underlying executor wrapped on a generic executor reference.
       */
      generic_executor& underlying_executor() BOOST_NOEXCEPT { return ex; }

    private:

      /**
       * The main loop of the worker thread
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

            boost::promise<void> p;
            try_executing_one_task tmp(task,p);
            ex.submit(tmp);
            p.get_future().wait();
          }
        }
        catch (...)
        {
          std::terminate();
          return;
        }
      }

    public:
      /// shared_state is not copyable.
      BOOST_THREAD_NO_COPYABLE(shared_state)

      /**
       * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
       *
       * \b Throws: Whatever exception is thrown while initializing the needed resources.
       */
      template <class Executor>
      shared_state(Executor& ex)
      : ex(ex), thr(&shared_state::worker_thread, this)
      {
      }
      /**
       * \b Effects: Destroys the thread pool.
       *
       * \b Synchronization: The completion of all the closures happen before the completion of the \c shared_state destructor.
       */
      ~shared_state()
      {
        // signal to the worker thread that there will be no more submissions.
        close();
      }

      /**
       * \b Effects: close the \c generic_serial_executor for submissions.
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
       * If invoked closure throws an exception the \c generic_serial_executor will call \c std::terminate, as is the case with threads.
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

    };

  public:

//    generic_serial_executor(generic_serial_executor const&) = default;
//    generic_serial_executor(generic_serial_executor &&) = default;

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    template <class Executor>
    generic_serial_executor(Executor& ex,
        typename boost::disable_if<is_same<typename decay<Executor>::type, generic_serial_executor>,
          int* >::type =  (int*)0)
    //: pimpl(make_shared<shared_state>(ex)) // // todo check why this doesn't works with C++03
    : pimpl(new shared_state(ex))
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c generic_serial_executor destructor.
     */
    ~generic_serial_executor()
    {
    }

    /**
     * \par Returns
     * The underlying executor wrapped on a generic executor reference.
     */
    generic_executor& underlying_executor() BOOST_NOEXCEPT
    {
      return pimpl->underlying_executor();
    }

    /**
     * \b Returns: always false as a serial executor can not re-enter.
     * Remark: A serial executor can not execute one of its pending tasks as the tasks depends on the other tasks.
     */
    bool try_executing_one()
    {
      return false;
    }

    /**
     * \b Effects: close the \c generic_serial_executor for submissions.
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
     * If invoked closure throws an exception the \c generic_serial_executor will call \c std::terminate, as is the case with threads.
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
     * \b Returns: always false as a serial executor can not re-enter.
     * Remark: A serial executor can not execute one of its pending tasks as the tasks depends on the other tasks.
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      return false;
    }
  private:
    shared_ptr<shared_state> pimpl;
  };
}
using executors::generic_serial_executor;
}

#include <boost/config/abi_suffix.hpp>

#endif
