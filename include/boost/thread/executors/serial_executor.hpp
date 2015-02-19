// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/11 Vicente J. Botet Escriba
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_SERIAL_EXECUTOR_HPP
#define BOOST_THREAD_SERIAL_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/config/abi_prefix.hpp>

//#define old_impl

namespace boost
{
namespace executors
{
class serial_executor : public boost::enable_shared_from_this<serial_executor>
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;

  private:
    typedef  scoped_thread<> thread_t;
    /// the thread safe work queue
#ifdef old_impl
	concurrent::sync_queue<work> work_queue;
#else
	concurrent::sync_queue<boost::function<void()> > work_queue;
#endif
    generic_executor_ref ex;
	boost::mutex mtx;
	boost::BOOST_THREAD_FUTURE<void> fut;
  public:
    /**
     * \par Returns
     * The underlying executor wrapped on a generic executor reference.
     */
    generic_executor_ref& underlying_executor() BOOST_NOEXCEPT { return ex; }

  private:

	 
	  /**
	  * Effects: try to execute one task.
	  * Returns: whether a task has been executed.
	  * Throws: whatever the current task constructor throws or the task() throws.
	  */
	  bool try_executing_one()
	  {
		  boost::lock_guard<decltype(mtx)> lockguard(mtx);
		  try
		  {
#ifdef old_impl
			  work task;
#else
			  boost::function<void()> task;
#endif
			  if (fut.is_ready() && (work_queue.try_pull(task) == queue_op_status::success))
			  {
				  auto task_with_cont = boost::bind<void>([](boost::weak_ptr<serial_executor> _spEx, boost::function<void()> w) -> void
				  {
					  w();
					  if (auto spEx = _spEx.lock())
					  {
						  spEx->try_executing_one();
					  }
				  }, this->shared_from_this(), boost::move(task));
				  boost::packaged_task<void()> ptask(boost::move(task_with_cont));
				  fut = ptask.get_future();
				  ex.submit(boost::move(ptask));
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

  public:
    /// serial_executor is not copyable.
    BOOST_THREAD_NO_COPYABLE(serial_executor)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    template <class Executor>
    serial_executor(Executor& ex)
		: ex(ex), fut(make_ready_future())
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c serial_executor destructor.
     */
    ~serial_executor()
    {
      // signal to the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c serial_executor for submissions.
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
      work_queue.push(work(closure));
	  try_executing_one();
    }
#endif
    void submit(void (*closure)())
    {
      work_queue.push(work(closure));
	  try_executing_one();
    }
#ifdef old_impl
    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      work_queue.push(work(boost::forward<Closure>(closure)));
	  try_executing_one();
    }
#else
	template <typename Closure>
	void submit(Closure closure)
	{
		work_queue.push(closure);
		try_executing_one();
	}
#endif

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
}
using executors::serial_executor;
}

#include <boost/config/abi_suffix.hpp>

#endif
