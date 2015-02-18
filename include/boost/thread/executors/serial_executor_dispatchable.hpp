// Copyright (C) 2015 Frank Schmitt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2015/02 Frank Schmitt
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_serial_executor_dispatchable_DISPATCHABLE_HPP
#define BOOST_THREAD_serial_executor_dispatchable_DISPATCHABLE_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  class serial_executor_dispatchable
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;
  private:


    /// the thread safe work queue
    sync_queue<work > work_queue;
    generic_executor_ref ex;

  public:
    /**
     * \par Returns
     * The underlying executor wrapped on a generic executor reference.
     */
    generic_executor_ref underlying_executor() BOOST_NOEXCEPT { return ex; }

	/**
	* Effects: try to execute one task.
	* Returns: whether a task has been executed.
	* Throws: whatever the current task constructor throws or the task() throws.
	*/
	bool try_executing_one(boost::BOOST_THREAD_FUTURE<void>& future)
	{
		work task;
		try
		{
			if (work_queue.try_pull(task) == queue_op_status::success)
			{
				boost::packaged_task<void()> tmp(task);
				future = tmp.get_future();
				ex.submit(std::move(tmp));
				return true;
			}
			return false;
		}
		catch (std::exception&)
		{
			return true; // we use true to indicate the dispatcher not to yield his thread and stop trying to dispatch others executors work
		}
		catch (...)
		{
			return true; // we use true to indicate the dispatcher not to yield his thread and stop trying to dispatch others executors work
		}
	}

  private:


  public:
    /// serial_executor_dispatchable is not copyable.
    BOOST_THREAD_NO_COPYABLE(serial_executor_dispatchable)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    template <class Executor>
    serial_executor_dispatchable(Executor& ex)
    : ex(ex)
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c serial_executor_dispatchable destructor.
     */
    ~serial_executor_dispatchable()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c serial_executor_dispatchable for submissions.
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
     * If invoked closure throws an exception the \c serial_executor_dispatchable will call \c std::terminate, as is the case with threads.
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
      work_queue.push_back(work(closure));
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
}
using executors::serial_executor_dispatchable;
}

#include <boost/config/abi_suffix.hpp>

#endif
