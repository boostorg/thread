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

#include <boost/thread/caller_context.hpp>
#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/executors/loop_executor.hpp>
#include <boost/thread/executors/generic_serial_executor.hpp>
#include <boost/thread/executors/serial_executor_threadless.hpp>
#include <boost/thread/executors/inline_executor.hpp>
#include <boost/thread/executors/thread_executor.hpp>
#include <boost/thread/executors/executor.hpp>
#include <boost/thread/executors/executor_adaptor.hpp>
#include <boost/thread/executor.hpp>
#include <boost/thread/future.hpp>
#include <boost/assert.hpp>
#include <string>
#include <iostream>

void p1()
{
  std::cout << BOOST_CONTEXTOF << std::endl;
  //boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
}

void p2()
{
  std::cout << BOOST_CONTEXTOF << std::endl;
  //boost::this_thread::sleep_for(boost::chrono::seconds(10));
}

int f1()
{
  // std::cout << BOOST_CONTEXTOF << std::endl;
  boost::this_thread::sleep_for(boost::chrono::seconds(1));
  return 1;
}
int f2(int i)
{
  // std::cout << BOOST_CONTEXTOF << std::endl;
  boost::this_thread::sleep_for(boost::chrono::seconds(2));
  return i + 1;
}

void submit_some(boost::executor& tp)
{
  for (int i = 0; i < 3; ++i) {
    tp.submit(&p2);
  }
  for (int i = 0; i < 3; ++i) {
    tp.submit(&p1);
  }

}


void at_th_entry(boost::basic_thread_pool )
{

}

int test_executor_adaptor()
{
  // std::cout << BOOST_CONTEXTOF << std::endl;
  {
    try
    {

#if ! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
		// std::cout << BOOST_CONTEXTOF << std::endl;

	   {
			boost::basic_thread_pool tp;
			boost::serial_executor_threadless e1(tp);
			boost::serial_executor_threadless e2 = e1;

	  }

	  {
		  boost::executor_adaptor < boost::inline_executor > ea1;
		  boost::executor_adaptor < boost::serial_executor_threadless > ea2(ea1);
		  submit_some(ea2);
	  }

	  {
		  boost::executor_adaptor < boost::basic_thread_pool > ea1(4);
		  boost::executor_adaptor < boost::serial_executor_threadless > ea2(ea1);
		  submit_some(ea2);
		  //boost::this_thread::sleep_for(boost::chrono::milliseconds(200));
	  }

#endif

      {
        boost::basic_thread_pool e1;
        boost::basic_thread_pool e2 = e1;
      }
      {
        boost::executor_adaptor < boost::basic_thread_pool > ea(4);
        submit_some( ea);
        {
          boost::future<int> t1 = boost::async(ea, &f1);
          boost::future<int> t2 = boost::async(ea, &f1);
          // std::cout << BOOST_CONTEXTOF << " t1= " << t1.get() << std::endl;
          // std::cout << BOOST_CONTEXTOF << " t2= " << t2.get() << std::endl;
        }
        submit_some(ea);
        {
          boost::basic_thread_pool ea3(1);
          boost::future<int> t1 = boost::async(ea3, &f1);
          boost::future<int> t2 = boost::async(ea3, &f1);
          //boost::future<int> t2 = boost::async(ea3, f2, 1); // todo this doesn't compiles yet on C++11
          //boost::future<int> t2 = boost::async(ea3, boost::bind(f2, 1)); // todo this doesn't compiles yet on C++98
          // std::cout << BOOST_CONTEXTOF << " t1= " << t1.get() << std::endl;
          // std::cout << BOOST_CONTEXTOF << " t2= " << t2.get() << std::endl;
        }
        submit_some(ea);
      }
      // std::cout << BOOST_CONTEXTOF << std::endl;
      {
        boost::loop_executor e1;
        boost::loop_executor e2 = e1;
        boost::executor_adaptor < boost::loop_executor > ea2(e2);
        submit_some( ea2);
        ea2.underlying_executor().run_queued_closures();
      }
      {
        boost::executor_adaptor < boost::loop_executor > ea2;
        submit_some( ea2);
        ea2.underlying_executor().run_queued_closures();
      }
      // std::cout << BOOST_CONTEXTOF << std::endl;
      {
        boost::basic_thread_pool tp;
        boost::generic_serial_executor e1(tp);
        boost::generic_serial_executor e2 = e1;
      }
      {
        boost::basic_thread_pool ea1(4);
        boost::generic_serial_executor ea2(ea1);
        boost::executor_adaptor < boost::generic_serial_executor > ea3(ea2);
        submit_some(ea3);
      }
      {
        boost::basic_thread_pool ea1(4);
        boost::generic_serial_executor ea2(ea1);
        boost::executor_adaptor < boost::generic_serial_executor > ea3(ea2);
        submit_some(ea3);
      }
//#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      {
        boost::basic_thread_pool ea1(4);
        boost::executor_adaptor < boost::generic_serial_executor > ea2(ea1);
        submit_some(ea2);
      }
//#endif
      // std::cout << BOOST_CONTEXTOF << std::endl;
      {
        boost::inline_executor e1;
        boost::inline_executor e2 = e1;
        boost::executor_adaptor < boost::inline_executor > ea2(e2);
        submit_some(ea2);
      }
      {
        boost::executor_adaptor < boost::inline_executor > ea1;
        submit_some(ea1);
      }
      // std::cout << BOOST_CONTEXTOF << std::endl;
      {
        boost::thread_executor e1;
        boost::thread_executor e2 = e1;
      }
      {
        boost::thread_executor e1;
        boost::executor_adaptor < boost::generic_executor > ea2(e1);
        submit_some(ea2);
      }

      {
        boost::executor_adaptor < boost::thread_executor > ea1;
        submit_some(ea1);
      }
      // std::cout << BOOST_CONTEXTOF << std::endl;
      {
        boost::basic_thread_pool  ea(4, at_th_entry);
        boost::future<int> t1 = boost::async(ea, &f1);
        // std::cout << BOOST_CONTEXTOF << " t1= " << t1.get() << std::endl;
      }
    }
    catch (std::exception& ex)
    {
      std::cout << "ERROR= " << ex.what() << "" << std::endl;
      return 1;
    }
    catch (...)
    {
      std::cout << " ERROR= exception thrown" << std::endl;
      return 2;
    }
  }
  // std::cout << BOOST_CONTEXTOF << std::endl;
  return 0;
}


int main()
{
  return test_executor_adaptor();
}
