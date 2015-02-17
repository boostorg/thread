// Copyright (C) 2015 Frank Schmitt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2015/02 Frank Schmitt
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_serial_executor_dispatcher_DISPATCHER_HPP
#define BOOST_THREAD_serial_executor_dispatcher_DISPATCHER_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>
//#include <boost/config/abi_prefix.hpp>
#include <boost/thread/executors/serial_executor_dispatchable.hpp>
#include <atomic>

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

	  typedef  scoped_thread<> thread_t;
	  thread_t thr;

    /// the registered serial_executor_dispatchable
	  typedef std::pair<std::shared_ptr<serial_executor_dispatchable>, boost::BOOST_THREAD_FUTURE<void>> dispatchable_with_future;
	  typedef std::list<dispatchable_with_future> registered_dispatchable_lst;
	  registered_dispatchable_lst registered_dispatchables;
	  std::atomic<bool> _closed;
	  boost::sync_queue< std::shared_ptr<boost::packaged_task<bool> > > work_queue;

  public:

	  boost::BOOST_THREAD_FUTURE<bool> add_serial_pool_executor(std::shared_ptr<serial_executor_dispatchable> spProxy)
	  {
		  boost::function<bool(std::shared_ptr<serial_executor_dispatchable>)> taskFunc = [&](std::shared_ptr<serial_executor_dispatchable> _spProxy) -> bool
		  {
			  if (!closed())
			  {
				  auto it = std::find_if(registered_dispatchables.begin(), registered_dispatchables.end(), [&](registered_dispatchable_lst::value_type& rVal) -> bool
				  {
					  return _spProxy.get() == rVal.first.get();
				  });
				  bool bNotFound = registered_dispatchables.end() == it;
				  if (bNotFound)
				  {
					  registered_dispatchables.push_back(std::make_pair(std::move(_spProxy), dispatchable_with_future::second_type()));
				  }
				  return bNotFound;
			  }
			  return false;
		  };

		  boost::function<bool(void)> boundedTaskFunc = boost::bind<bool>(taskFunc, spProxy);
		  std::shared_ptr<boost::packaged_task<bool>> task = std::make_shared<boost::packaged_task<bool>>(boundedTaskFunc);

		  boost::BOOST_THREAD_FUTURE<bool> fut = task->get_future();
		  work_queue.push(std::move(task));
		  return std::move(fut);
	  }

	  boost::BOOST_THREAD_FUTURE<bool> remove_serial_pool_executor(std::shared_ptr<serial_executor_dispatchable> spProxy)
	  {
		  boost::function<bool(std::shared_ptr<serial_executor_dispatchable>)> taskFunc = [&](std::shared_ptr<serial_executor_dispatchable> _spProxy) -> bool
		  {
			  auto it = std::find_if(registered_dispatchables.begin(), registered_dispatchables.end(), [&](registered_dispatchable_lst::value_type& rVal) -> bool
			  {
				  return _spProxy.get() == rVal.first.get();
			  });
			  bool bSuccess = registered_dispatchables.end() != it;
			  if (bSuccess)
			  {
				  registered_dispatchables.erase(it);
			  }
			  return bSuccess;
		  };


		  boost::function<bool(void)> boundedTaskFunc = boost::bind<bool>(taskFunc, spProxy);
		  std::shared_ptr<boost::packaged_task<bool>> task = std::make_shared<boost::packaged_task<bool>>(boundedTaskFunc);

		  boost::BOOST_THREAD_FUTURE<bool> fut = task->get_future();
		  work_queue.push(std::move(task));
		  return std::move(fut);
	  }

	  void join()
	  {
		  thr.join();
	  }



	  bool try_executing()
	  {
		  bool bHaveDoneSomeWork = !registered_dispatchables.empty();
		  for (auto& rEntry : registered_dispatchables)
		  {
			  const auto& rFut = rEntry.second;
			  if (!rFut.valid() || rFut.is_ready())
			  {
				  bHaveDoneSomeWork &= try_executing_one(rEntry);
			  }
		  }
		  return bHaveDoneSomeWork;
	  }

  private:

	  bool try_executing_one(dispatchable_with_future& rTaskContainer)
	  {
		  try
		  {
			  boost::BOOST_THREAD_FUTURE<void> fut;
			  if (rTaskContainer.first->try_executing_one(fut))
			  {
				  rTaskContainer.second = std::move(fut);
				  return true;
			  }
			  return false;
		  }
		  catch (std::exception& ex)
		  {
			  std::cout << ex.what() << std::endl;
			  return true; // return true to keep continuing checking the other register serializer
		  }
		  catch (...)
		  {
			  return true; // return true to keep continuing checking the other register serializer
		  }
	  }

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
			//add other proxys here as local functors
			std::shared_ptr<boost::packaged_task<bool>> spTask;
			while (work_queue.try_pull_front(spTask) == boost::queue_op_status::success)
			{
				(*spTask)();
			}
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
	  join();
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

//#include <boost/config/abi_suffix.hpp>

#endif
