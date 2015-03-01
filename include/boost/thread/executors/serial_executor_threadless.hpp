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
#include <atomic>
#include <future>
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
			  : ex(ex), sp_fut()
		  {

		  }

		  /// the thread safe work queue
		  concurrent::sync_queue<work> work_queue;
		  generic_executor_ref ex;
		  boost::recursive_mutex mtx;
		  //boost::mutex mtx;
		  boost::shared_ptr<boost::BOOST_THREAD_FUTURE<void>> sp_fut;

		  struct try_executing_one_task {
			  work task;
			  boost::function<void(void)> followup_task;
			  boost::shared_ptr<boost::promise<void>> p;
			  boost::shared_ptr<boost::BOOST_THREAD_FUTURE<void>> sp_fut;

			  try_executing_one_task(try_executing_one_task&& other)
				  : task()
				  , followup_task()
				  , p()
				  , sp_fut()
			  {
				  task = boost::move(other.task);
				  followup_task = boost::move(other.followup_task);
				  p.swap(other.p);
				  sp_fut.swap(other.sp_fut);
				  //std::cout << "move ctor " << this << std::endl;
			  }

			  try_executing_one_task(const try_executing_one_task& rOther)
				  : task(rOther.task)
				  , followup_task(rOther.followup_task)
				  , p(rOther.p)
				  , sp_fut(rOther.sp_fut)
			  {
				  //std::cout << "copy ctor " << this << std::endl;
			  }

			  try_executing_one_task(work _task, boost::function<void(void)> ftask)
				  : task(boost::move(_task))
				  , followup_task(ftask)
				  , p(new boost::promise<void>())
				  , sp_fut(new boost::BOOST_THREAD_FUTURE<void>(boost::move(p->get_future())))
			  {
				  //std::cout << "ctor " << this << std::endl;
			  }

			  //try_executing_one_task& operator=(const try_executing_one_task& rOther)
			  //{
				 // if (&rOther != this)
				 // {
					//  task = rOther.task;
					//  followup_task = rOther.followup_task;
					//  p = rOther.p;
					//  sp_fut = rOther.sp_fut;
				 // }
				 // return *this;
			  //}

			  //try_executing_one_task& operator=(try_executing_one_task&& other)
			  //{
				 // if (&other != this)
				 // {
					//  task = boost::move(other.task);
					//  followup_task = boost::move(other.followup_task);
					//  p.swap(other.p);
					//  sp_fut.swap(other.sp_fut);
				 // }
				 // return *this;
			  //}

			  ~try_executing_one_task()
			  {
				  //std::cout << "dtor " << this << std::endl;
				  if (sp_fut && !sp_fut->has_value()) // prevent broken promise exception, there is surely a better way...
				  {
					  p->set_value();
				  }
				  sp_fut.reset();
				  p.reset();
			  }

			  boost::shared_ptr<boost::BOOST_THREAD_FUTURE<void>> get_future(){
				  
				  return sp_fut;
			  }

			  void operator()() {
				  try {
					  //std::cout << "callable " << this << std::endl;
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
				  if ((!sp_fut || sp_fut->is_ready()) && (work_queue.try_pull(task) == queue_op_status::success))
				  {
					  auto task_cont = boost::bind<void>([](boost::weak_ptr<serial_executor_threadless_impl> _spEx) -> void
					  {
						  if (auto spEx = _spEx.lock())
						  {
							  spEx->try_executing_one();
						  }
					  }, this->shared_from_this());

					  try_executing_one_task tmp(boost::move(task), task_cont);
					  sp_fut = tmp.get_future();
					  //don't move here
					  //ex.submit(boost::move(tmp));

					  //and this is also not so good...
					  //work w(boost::move(tmp));
					  //ex.submit(boost::move(w));

					  //this works but makes a lot of copies down the road
					  ex.submit(tmp);

					  return true;
				  }
				  return false;
			  }
			  catch (boost::concurrent::sync_queue_is_closed& /*exp*/)
			  {
				  //std::cout << "associated executor already closed..." << std::endl;
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

		  template <typename Closure>
		  void submit(Closure closure)
		  {
			  work_queue.push(work(closure));
			  try_executing_one();
		  }

		  void submit(void(*closure)())
		  {
			  work_queue.push(work(closure));
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


	template <typename Closure>
	void submit(Closure closure)
	{
		impl->submit(boost::move(closure));
	}

	void submit(void(*closure)())
	{
		impl->submit(boost::move(closure));
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
