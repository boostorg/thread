// Copyright (C) 2015 Frank Schmitt
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2015/02 Frank Schmitt
//    first implementation of a simple serial scheduler.

#ifndef BOOST_THREAD_CYCLIC_EXECUTOR_DISPATCHABLE_HPP
#define BOOST_THREAD_CYCLIC_EXECUTOR_DISPATCHABLE_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/sync_queue.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/executors/generic_executor_ref.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <atomic>
#include <boost/config/abi_prefix.hpp>

namespace boost
{
	namespace executors
	{
		class cyclic_executor_dispatchable
		{

			typedef boost::function<void(void)> work;

			typedef boost::chrono::milliseconds time_duration_ms;
		private:

			std::list<work> cyclicwork_container;
			std::list<work>::iterator current_cyclicwork_item;
			std::atomic<time_duration_ms> min_wait_duration_between_calls_in_ms;
			boost::chrono::high_resolution_clock::time_point next_trigger_timepoint;
			generic_executor_ref ex;
			bool _closed;

		public:

			template <class Executor, class WorkContainer>
			cyclic_executor_dispatchable(Executor& _ex, WorkContainer wc)
				: ex(_ex)
				, _closed(_ex.closed())
				, min_wait_duration_between_calls_in_ms()
				, next_trigger_timepoint(boost::chrono::high_resolution_clock::now())
			{
				std::copy(wc.begin(), wc.end(), std::back_inserter(cyclicwork_container));
				current_cyclicwork_item = cyclicwork_container.begin();
			}

			void set_min_duration_between_calls_in_ms(time_duration_ms dur)
			{
				min_wait_duration_between_calls_in_ms = dur;
			}

			void close()
			{
				_closed = true;
				//work_queue.close();
			}

			/**
			* \b Returns: whether the pool is closed for submissions.
			*/
			bool closed()
			{
				return _closed;
			}

			bool try_executing_one(boost::BOOST_THREAD_FUTURE<void>& future)
			{
				//check container
				if (cyclicwork_container.empty() || _closed)
				{
					return false;
				}
				//check for timeduration 0
				time_duration_ms minwait = min_wait_duration_between_calls_in_ms;
				bool bTriggerTime = minwait.count() == 0;
				if (!bTriggerTime) //check for timeduration
				{
					auto now = boost::chrono::high_resolution_clock::now();
					if (next_trigger_timepoint < now)
					{
						next_trigger_timepoint = now + minwait;
						bTriggerTime = true;
					}
				}

				if (bTriggerTime)
				{
					//schedule task //copy current work
					boost::packaged_task<void()> tmp(*current_cyclicwork_item);
					future = tmp.get_future();
					ex.submit(std::move(tmp));

					// increment
					++current_cyclicwork_item;
					// check for end
					if (cyclicwork_container.end() == current_cyclicwork_item)
					{
						// set begin to next element
						current_cyclicwork_item = cyclicwork_container.begin();
					}

					return true;
				}
				return false;
			}
		};
	}
	using executors::cyclic_executor_dispatchable;
}

#include <boost/config/abi_suffix.hpp>

#endif