// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_PROVIDES_EXECUTORS

#include <iostream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>

#include <boost/detail/lightweight_test.hpp>

using namespace boost::chrono;

typedef boost::concurrent::sync_priority_queue<int> sync_pq;

void test_pull_for()
{
  sync_pq pq;
  steady_clock::time_point start = steady_clock::now();
  boost::optional<int> val = pq.pull_for(milliseconds(500));
  steady_clock::duration diff = steady_clock::now() - start;
  BOOST_TEST(!val);
  BOOST_TEST(diff < milliseconds(510) && diff > milliseconds(500));
}

void test_pull_until()
{
  sync_pq pq;
  steady_clock::time_point start = steady_clock::now();
  boost::optional<int> val = pq.pull_until(start + milliseconds(500));
  steady_clock::duration diff = steady_clock::now() - start;
  BOOST_TEST(!val);
  BOOST_TEST(diff < milliseconds(510) && diff > milliseconds(500));
}

void test_pull_no_wait()
{
  sync_pq pq;
  steady_clock::time_point start = steady_clock::now();
  boost::optional<int> val = pq.pull_no_wait();
  steady_clock::duration diff = steady_clock::now() - start;
  BOOST_TEST(!val);
  BOOST_TEST(diff < milliseconds(5));
}

void test_pull_for_when_not_empty()
{
  sync_pq pq;
  pq.push(1);
  steady_clock::time_point start = steady_clock::now();
  boost::optional<int> val = pq.pull_for(milliseconds(500));
  steady_clock::duration diff = steady_clock::now() - start;
  BOOST_TEST(val);
  BOOST_TEST(diff < milliseconds(5));
}

void test_pull_until_when_not_empty()
{
  sync_pq pq;
  pq.push(1);
  steady_clock::time_point start = steady_clock::now();
  boost::optional<int> val = pq.pull_until(start + milliseconds(500));
  steady_clock::duration diff = steady_clock::now() - start;
  BOOST_TEST(val);
  BOOST_TEST(diff < milliseconds(5));
}

int main()
{
  sync_pq pq;
  BOOST_TEST(pq.empty());
  BOOST_TEST(!pq.closed());
  BOOST_TEST_EQ(pq.size(), 0);

  for(int i = 1; i <= 5; i++){
    pq.push(i);
    BOOST_TEST(!pq.empty());
    BOOST_TEST_EQ(pq.size(), i);
  }

  for(int i = 6; i <= 10; i++){
    boost::queue_op_status succ = pq.try_push(i);
    BOOST_TEST(succ == boost::queue_op_status::success );
    BOOST_TEST(!pq.empty());
    BOOST_TEST_EQ(pq.size(), i);
  }

  for(int i = 10; i > 5; i--){
    int val = pq.pull();
    BOOST_TEST_EQ(val, i);
  }

  for(int i = 5; i > 0; i--){
    boost::optional<int> val = pq.try_pull();
    BOOST_TEST(val);
    BOOST_TEST_EQ(*val, i);
  }

  BOOST_TEST(pq.empty());
  pq.close();
  BOOST_TEST(pq.closed());

  test_pull_for();
  test_pull_until();
  test_pull_no_wait();

  test_pull_for_when_not_empty();
  test_pull_until_when_not_empty();

  return boost::report_errors();
}
