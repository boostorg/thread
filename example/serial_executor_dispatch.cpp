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
#include <boost/thread/executors/cyclic_executor_dispatchable.hpp>
#include <boost/thread/executors/serial_executor.hpp>
#include <boost/thread/executors/thread_executor.hpp>
#include <boost/thread/executors/executor.hpp>
#include <boost/thread/executors/executor_adaptor.hpp>
#include <boost/thread/executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/assert.hpp>
#include <string>
#include <iostream>
#include <mutex>

int test_serial_ex()
{
	boost::executors::basic_thread_pool t_pool;

	std::list<boost::shared_ptr<boost::executors::serial_executor>> lst_serial_executor;
	const size_t num_dispatchables = 50;
	for (size_t i = 0; i < num_dispatchables; ++i)
	{
		auto sp_ex = boost::make_shared<boost::executors::serial_executor>(t_pool);
		lst_serial_executor.push_back(sp_ex);
	}

	std::atomic<size_t> callcount = 0;
	static std::mutex mtx;
	auto work = std::bind([&](boost::chrono::milliseconds duration, size_t index_ex, size_t index_work) -> void {
		std::lock_guard<decltype(mtx)> lock(mtx);

		lst_serial_executor.front()->submit(boost::bind<void>([](size_t index_ex, size_t index_work, size_t callcount)
		{
			std::cout << "call" << callcount << " in work from " << index_ex << "," << index_work << std::endl;
		}, index_ex, index_work, callcount++)); // submit to the first serializer to have threadsafe output...

		boost::chrono::high_resolution_clock::time_point end = boost::chrono::high_resolution_clock::now() + duration;
		while (end > boost::chrono::high_resolution_clock::now());
	}, boost::chrono::milliseconds(1), std::placeholders::_1, std::placeholders::_2);

	std::atomic<size_t> count = 0;
	for (const auto& rSpSerialEx : lst_serial_executor)
	{
		++count;
		const size_t num_tasks = 10;
		for (size_t i = 0; i < num_tasks; ++i)
		{
			boost::function<void(void)> func = boost::bind<void>(work, (int)count, i);
			auto bound = boost::bind<void>([](boost::shared_ptr<boost::executors::serial_executor> spEx, boost::function<void(void)> func_){ spEx->submit(boost::move(func_)); }, rSpSerialEx, func);
			boost::async(bound); // make this a bit concurrently			
		}		
	}

	boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));

	std::cout << callcount << std::endl;

	return 0;

}

int test_dispatcher()
{
	boost::executors::basic_thread_pool t_pool;
	auto sp_dispatcher = std::make_shared<boost::executors::serial_executor_dispatcher>();

	std::list<std::shared_ptr<boost::executors::serial_executor_dispatchable>> lst_serial_executor;
	const size_t num_dispatchables = 50;
	for (size_t i = 0; i < num_dispatchables; ++i)
	{
		auto sp_ex = std::make_shared<boost::executors::serial_executor_dispatchable>(t_pool);
		lst_serial_executor.push_back(sp_ex);
		sp_dispatcher->add_dispatchable_executor(sp_ex);
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

  return 0;
}

int test_cyclic()
{
	boost::executors::basic_thread_pool pool;
	auto sp_dispatcher = std::make_shared<boost::executors::serial_executor_dispatcher>();

	double avg = 0.0;
	size_t callcount = 0;
	boost::chrono::high_resolution_clock::time_point last_time;
	auto cylicwork = [&]()
	{
		auto cur_time = boost::chrono::high_resolution_clock::now();

		if (last_time.time_since_epoch().count() == 0)
		{
			std::cout << "measuring begins..." << std::endl;
		}
		else
		{		
			double dur = boost::chrono::duration_cast<boost::chrono::microseconds>(cur_time - last_time).count() / 1000.0;
			std::cout << "time in between was " << dur << std::endl;
			avg += dur;
		}
		last_time = cur_time;
		++callcount;
	};

	std::vector<std::function<void(void)>> lstWork;
	lstWork.push_back(cylicwork);

	auto spProxy = std::make_shared < boost::executors::cyclic_executor_dispatchable >(pool, lstWork);
	spProxy->set_min_duration_between_calls_in_ms(boost::chrono::milliseconds(20));
	sp_dispatcher->add_dispatchable_executor(spProxy);

	
	boost::this_thread::sleep_for(boost::chrono::milliseconds(2000));

	spProxy.reset();
	sp_dispatcher.reset();

	boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

	std::cout << "callcount= " << callcount << ". avg time in between was " << avg / callcount << std::endl;

	return 0;
}

int main()
{
	return test_serial_ex() & test_dispatcher() & test_cyclic();
}
