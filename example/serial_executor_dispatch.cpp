// Copyright (C) 2012-2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_PROVIDES_EXECUTORS
//#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID
#define BOOST_THREAD_QUEUE_DEPRECATE_OLD

//#undef BOOST_THREAD_PROVIDES_SIGNATURE_PACKAGED_TASK

#include <boost/thread/caller_context.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/executors/loop_executor.hpp>
#include <boost/thread/executors/serial_executor_dispatcher.hpp>
#include <boost/thread/executors/serial_executor_dispatchable.hpp>
#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/executors/thread_executor.hpp>
#include <boost/thread/executors/executor.hpp>
#include <boost/thread/executors/executor_adaptor.hpp>
#include <boost/thread/executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/assert.hpp>
#include <string>
#include <iostream>


int test_executer()
{
	boost::executors::basic_thread_pool t_pool;
	auto sp_dispatcher = std::make_shared<boost::executors::serial_executor_dispatcher>();

	std::list<std::shared_ptr<boost::executors::serial_executor_dispatchable>> lst_serial_executor;
	const size_t num_dispatchables = 50;
	for (size_t i = 0; i < num_dispatchables; ++i)
	{
		auto sp_ex = std::make_shared<boost::executors::serial_executor_dispatchable>(t_pool);
		lst_serial_executor.push_back(sp_ex);
		sp_dispatcher->add_serial_pool_executor(sp_ex);
	}

	std::atomic<size_t> callcount = 0;

	// create some work
	auto work = std::bind([&](boost::chrono::milliseconds duration) -> void {
		++callcount;
		boost::chrono::high_resolution_clock::time_point end = boost::chrono::high_resolution_clock::now() + duration;
		while (end > boost::chrono::high_resolution_clock::now());
	}, boost::chrono::milliseconds(20));

	for (const auto& rSpSerialEx : lst_serial_executor)
	{
		const size_t num_tasks = 10;
		for (size_t i = 0; i < num_tasks; ++i)
		{
			rSpSerialEx->submit(work);
		}
	}

	boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));

	sp_dispatcher.reset();

	std::cout << callcount << std::endl;

  // std::cout << BOOST_CONTEXTOF << std::endl;
  return 0;
}


int main()
{
	return test_executer();
}
