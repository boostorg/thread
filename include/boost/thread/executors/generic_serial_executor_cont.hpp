// Copyright (C) 2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_GENERIC_SERIAL_EXECUTOR_CONT_HPP
#define BOOST_THREAD_GENERIC_SERIAL_EXECUTOR_CONT_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  class generic_serial_executor_cont
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:

    struct shared_state {
      typedef  executors::work work;

    generic_executor ex_;
    future<void> fut_; // protected by mtx_
    bool closed_; // protected by mtx_
    mutex mtx_;

    struct continuation {
      work task;
      template <class X>
      struct result {
        typedef void type;
      };
      continuation(BOOST_THREAD_RV_REF(work) tsk)
      : task(boost::move(tsk)) {}
      void operator()(future<void> f)
      {
        try {
          task();
        } catch (...)  {
          std::terminate();
        }
      }
    };

    bool closed(lock_guard<mutex>&) const
    {
      return closed_;
    }
  public:
    /**
     * \par Returns
     * The underlying executor wrapped on a generic executor reference.
     */
    generic_executor& underlying_executor() BOOST_NOEXCEPT { return ex_; }

    /// shared_state is not copyable.
    BOOST_THREAD_NO_COPYABLE(shared_state)

    /**
     * \b Effects: creates a serial executor that runs closures in fifo order using one the associated executor.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     *
     * \b Notes:
     * * The lifetime of the associated executor must outlive the serial executor.
     * * The current implementation doesn't support submission from synchronous continuation, that is,
     *     - the executor must execute the continuation asynchronously or
     *     - the continuation can not submit to this serial executor.
     */
    template <class Executor>
    shared_state(Executor& ex)
    : ex_(ex), fut_(make_ready_future()), closed_(false)
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c generic_serial_executor_cont destructor.
     */
    ~shared_state()
    {
      // signal to the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c generic_serial_executor_cont for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
      lock_guard<mutex> lk(mtx_);
      closed_ = true;;
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
      lock_guard<mutex> lk(mtx_);
      return closed(lk);
    }

    /**
     * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
     *
     * \b Effects: The specified \c closure will be scheduled for execution after the last submitted closure finish.
     * If the invoked closure throws an exception the \c generic_serial_executor_cont will call \c std::terminate, as is the case with threads.
     *
     * \b Throws: \c sync_queue_is_closed if the executor is closed.
     * Whatever exception that can be throw while storing the closure.
     *
     */

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))       BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      fut_ = fut_.then(ex_, continuation(work(closure)));
    }
#endif
    void submit(void (*closure)())
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))       BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      fut_ = fut_.then(ex_, continuation(work(closure)));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      lock_guard<mutex> lk(mtx_);
      if (closed(lk))       BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
      fut_ = fut_.then(ex_, continuation(work(boost::forward<Closure>(closure))));
    }
    };
  public:
    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    template <class Executor>
    generic_serial_executor_cont(Executor& ex)
    : pimpl(make_shared<shared_state>(ex))
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c serial_executor destructor.
     */
    ~generic_serial_executor_cont()
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
     * \b Effects: close the \c serial_executor for submissions.
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
     * If invoked closure throws an exception the \c serial_executor will call \c std::terminate, as is the case with threads.
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
using executors::generic_serial_executor_cont;
}

#include <boost/config/abi_suffix.hpp>

#endif
