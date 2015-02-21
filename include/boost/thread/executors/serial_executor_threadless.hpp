// Copyright (C) 2015 Frank Schmitt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2015/02 Frank Schmitt
//    first implementation of a serial executor

#ifndef BOOST_THREAD_serial_executor_threadless_HPP
#define BOOST_THREAD_serial_executor_threadless_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{

class serial_executor_threadless 
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;

  private:

	  struct serial_executor_threadless_impl : public boost::enable_shared_from_this < serial_executor_threadless_impl>
	  {
		  template <class Executor>
		  serial_executor_threadless_impl(Executor& ex)
			  : ex(ex), fut(make_ready_future())
		  {

		  }

		  /// the thread safe work queue
		  concurrent::sync_queue<work> work_queue;
		  generic_executor_ref ex;
		  boost::recursive_mutex mtx;
		  boost::BOOST_THREAD_FUTURE<void> fut;

		  struct try_executing_one_task {
			  work task;
			  boost::function<void(void)> followup_task;
			  boost::shared_ptr<boost::promise<void>> p;
			  try_executing_one_task(work task, boost::function<void(void)> ftask)
				  : task(boost::move(task)), followup_task(ftask), p(new boost::promise<void>()){}

			  boost::BOOST_THREAD_FUTURE<void> get_future(){
				  return p->get_future();
			  }

			  void operator()() {
				  try {
					  task();
					  p->set_value();
					  followup_task();
				  }
				  catch (...)
				  {
					  p->set_exception(boost::current_exception());
				  }
			  }
		  };

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
		  * Effects: try to execute one task.
		  * Returns: whether a task has been executed.
		  * Throws: whatever the current task constructor throws or the task() throws.
		  */
		  bool try_executing_one()
		  {
			  boost::lock_guard<decltype(mtx)> lockguard(mtx);
			  try
			  {
				  work task;
				  if (fut.is_ready() && (work_queue.try_pull(task) == queue_op_status::success))
				  {
					  auto task_cont = boost::bind<void>([](boost::weak_ptr<serial_executor_threadless_impl> _spEx) -> void
					  {
						  if (auto spEx = _spEx.lock())
						  {
							  spEx->try_executing_one();
						  }
					  }, this->shared_from_this());

					  try_executing_one_task tmp(boost::move(task), task_cont);
					  fut = tmp.get_future();
					  ex.submit(boost::move(tmp));

					  return true;
				  }
				  return false;
			  }
			  catch (boost::concurrent::sync_queue_is_closed& /*exp*/)
			  {
				  std::cout << "associated executor already closed..." << std::endl;
				  return false;
			  }
			  catch (...)
			  {				  
				  std::terminate();
				  return false;
			  }
		  }

		  /**
		  * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
		  *
		  * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
		  * If invoked closure throws an exception the \c serial_executor_threadless will call \c std::terminate, as is the case with threads.
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
		  void submit(void(*closure)())
		  {
			  work_queue.push(work(closure));
			  try_executing_one();
		  }

		  template <typename Closure>
		  void submit(BOOST_THREAD_RV_REF(Closure) closure)
		  {
			  work_queue.push(work(boost::forward<Closure>(closure)));
			  try_executing_one();
		  }

		  void submit(BOOST_THREAD_RV_REF(work) closure)  {
			  work_queue.push(work(boost::forward<work>(closure)));
			  try_executing_one();
		  }

	  };



	  boost::shared_ptr<serial_executor_threadless_impl> impl;

  public:
    /**
     * \par Returns
     * The underlying executor wrapped on a generic executor reference.
     */
	  generic_executor_ref& underlying_executor() BOOST_NOEXCEPT{ return impl->ex; }


		/**
		* Effects: try to execute one task.
		* Returns: whether a task has been executed.
		* Throws: whatever the current task constructor throws or the task() throws.
		*/
		bool try_executing_one()
		{
			return impl->try_executing_one();
		}
	 
  public:
    /// serial_executor_threadless is not copyable.
    BOOST_THREAD_NO_COPYABLE(serial_executor_threadless)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */

	template <class Executor>
    serial_executor_threadless(Executor& ex)
		: impl(new serial_executor_threadless_impl(ex))
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c serial_executor_threadless destructor.
     */
    ~serial_executor_threadless()
    {
      // signal to the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c serial_executor_threadless for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
		impl->close();
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
      return impl->closed();
    }

    /**
     * \b Requires: \c Closure is a model of \c Callable(void()) and a model of \c CopyConstructible/MoveConstructible.
     *
     * \b Effects: The specified \c closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the \c serial_executor_threadless will call \c std::terminate, as is the case with threads.
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
		impl->submit(boost::move(closure));
	}
#endif
	void submit(void(*closure)())
	{
		impl->submit(boost::move(closure));
	}

	template <typename Closure>
	void submit(BOOST_THREAD_RV_REF(Closure) closure)
	{
		impl->submit(boost::forward<Closure>(closure));
	}

	void submit(BOOST_THREAD_RV_REF(work) closure)  {
		impl->submit(boost::forward<work>(closure));
	}

  };
}
using executors::serial_executor_threadless;
}

#include <boost/config/abi_suffix.hpp>

#endif
