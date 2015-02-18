// Copyright (C) 2015 Frank Schmitt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2015/02 Frank Schmitt
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_SERIAL_EXECUTOR_DISPATCHER_HPP
#define BOOST_THREAD_SERIAL_EXECUTOR_DISPATCHER_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/executors/serial_executor_dispatchable.hpp>
#include <atomic>
#include <boost/config/abi_prefix.hpp>


namespace boost
{
namespace executors 
{
  class serial_executor_dispatcher
  {
  public:
    /// type-erasure to store the works to do
    typedef  executors::work work;

  private:

    /// the registered serial_executor_dispatchable
	  typedef std::function<int(boost::BOOST_THREAD_FUTURE<void>& rFuture)> func_try_submit_one;
	  typedef std::pair<func_try_submit_one, boost::BOOST_THREAD_FUTURE<void>>  func_try_submit_one_with_fut;
	  typedef std::list<func_try_submit_one_with_fut> lst_registered_dispatchable;
	  lst_registered_dispatchable registered_dispatchable;

	  std::atomic<bool> _closed;
	  boost::sync_queue< std::shared_ptr<boost::packaged_task<bool()> > > work_queue;
	  typedef  scoped_thread<> thread_t;
	  thread_t thr;

  public:

	  template <class T>
	  boost::BOOST_THREAD_FUTURE<bool> add_dispatchable_executor(std::shared_ptr<T> spExecutor) //overload with weak_ptr
	  {
		  auto funcTrySubmitOne = [](std::weak_ptr<T> wpEx, boost::BOOST_THREAD_FUTURE<void>& rFuture) -> int
		  {
			  if (auto spEx = wpEx.lock())
			  {
				  return spEx->try_executing_one(rFuture); // 0 nothing to do // 1 have done something
			  }
			  else
			  {
				  return -1; // pointer deleted
			  }
		  };

		  std::function<int(boost::BOOST_THREAD_FUTURE<void>& rFuture)> boundFuncTrySubmitOne = std::bind(funcTrySubmitOne, spExecutor, std::placeholders::_1);

		  auto taskFunc = [&](func_try_submit_one _f) -> bool
		  {
			  if (!_closed)
			  {
				  registered_dispatchable.push_back(func_try_submit_one_with_fut(std::move(_f), func_try_submit_one_with_fut::second_type()));
				  return true;
			  }
			  return false;
		  };

		  boost::function<bool(void)> boundedTaskFunc = boost::bind<bool>(taskFunc, std::move(boundFuncTrySubmitOne));
		  std::shared_ptr<boost::packaged_task<bool()>> task = std::make_shared<boost::packaged_task<bool()>>(std::move(boundedTaskFunc));
		  boost::BOOST_THREAD_FUTURE<bool> fut = task->get_future();
		  work_queue.push(std::move(task));
		  return fut;
	  }

	bool try_executing()
	{
		bool bNothingTodo = registered_dispatchable.empty();

		for (auto& rEntry : registered_dispatchable)
		{
			auto& rFut = rEntry.second;

			if (!rFut.has_value() || rFut.is_ready())
			{
				int iRes = rEntry.first(rFut);
				if (-1 == iRes)
				{
					//todo deregister automatically
				}
				else
				{
					bNothingTodo |= !(iRes);
				}
			}
		}

		return !bNothingTodo;
	}

  private:


    /**
     * Effects: schedule one task or yields
     * Throws: whatever the current task constructor throws or the task() throws.
     */
	  void schedule_or_yield()
    {
		if (!try_executing())
		{
			boost::this_thread::yield();
		}
    }

    /**
     * The main loop of the worker thread
     */
    void worker_thread()
    {
		while (!closed())
		{
			//1. process own register/unregister tasks
			std::shared_ptr<boost::packaged_task<bool()>> spTask;
			while (work_queue.try_pull(spTask) == boost::queue_op_status::success)
			{
				(*spTask)();
			}
			//2. do dispatcher work
			schedule_or_yield();
		}

		while (try_executing())
		{
		}
    }

  public:
    /// serial_executor_dispatcher is not copyable.
    BOOST_THREAD_NO_COPYABLE(serial_executor_dispatcher)

    /**
     * \b Effects: creates a thread pool that runs closures using one of its closure-executing methods.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    serial_executor_dispatcher()
    :thr(&serial_executor_dispatcher::worker_thread, this)
	, _closed(false)
    {
    }
    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c serial_executor_dispatcher destructor.
     */
    ~serial_executor_dispatcher()
    {
      // signal to all the worker thread that there will be no more submissions.
      close();
    }

    /**
     * \b Effects: close the \c serial_executor_dispatcher for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
		_closed = true;
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
		return _closed;
    }
  };
}
using executors::serial_executor_dispatcher;
}

#include <boost/config/abi_suffix.hpp>

#endif
