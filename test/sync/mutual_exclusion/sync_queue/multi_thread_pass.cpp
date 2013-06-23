// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/sync_queue.hpp>

// class sync_queue<T>

//    push || pull;

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4

#include <boost/thread/sync_queue.hpp>
#include <boost/thread/thread_only.hpp>
#include <boost/thread/future.hpp>

#include <boost/detail/lightweight_test.hpp>

struct call_push
{
  boost::sync_queue<int> &q_;
  boost::shared_future<void> ready_;
  boost::promise<void> &op_ready_;

  call_push(boost::sync_queue<int> &q, boost::shared_future<void> ready, boost::promise<void> &op_ready) :
    q_(q), ready_(ready), op_ready_(op_ready)
  {
  }
  typedef void result_type;
  void operator()()
  {
    op_ready_.set_value();
    ready_.wait();
    q_.push(42);
  }

};

struct call_pull
{
  boost::sync_queue<int> &q_;
  boost::shared_future<void> ready_;
  boost::promise<void> &op_ready_;

  call_pull(boost::sync_queue<int> &q, boost::shared_future<void> ready, boost::promise<void> &op_ready) :
    q_(q), ready_(ready), op_ready_(op_ready)
  {
  }
  typedef int result_type;
  int operator()()
  {
    op_ready_.set_value();
    ready_.wait();
    return q_.pull();
  }

};

void test_concurrent_push_and_pull_on_empty_queue()
{
  boost::sync_queue<int> q;

  boost::promise<void> go, push_ready, pull_ready;
  boost::shared_future<void> ready(go.get_future());

  boost::future<void> push_done;
  boost::future<int> pull_done;

  try
  {
    push_done=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,ready,&push_ready]()
        {
          push_ready.set_value();
          ready.wait();
          q.push(42);
        }
#else
        call_push(q,ready,push_ready)
#endif
    );
    pull_done=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,ready,&pull_ready]()
        {
          pull_ready.set_value();
          ready.wait();
          return q.pull();
        }
#else
        call_pull(q,ready,pull_ready)
#endif
    );
    push_ready.get_future().wait();
    pull_ready.get_future().wait();
    go.set_value();

    push_done.get();
    BOOST_TEST_EQ(pull_done.get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    go.set_value();
    BOOST_TEST(false);
  }
}

void test_concurrent_push_on_empty_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::promise<void> go;
  boost::shared_future<void> ready(go.get_future());

  boost::promise<void> push_ready[n];
  boost::future<void> push_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      push_done[i]=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,ready,&push_ready,i]()
        {
          push_ready[i].set_value();
          ready.wait();
          q.push(42);
        }
#else
        call_push(q,ready,push_ready[i])
#endif
    );

    for (unsigned int i = 0; i < n; ++i)
      push_ready[i].get_future().wait();
    go.set_value();

    for (unsigned int i = 0; i < n; ++i)
      push_done[i].get();
    BOOST_TEST(!q.empty());
    for (unsigned int i =0; i< n; ++i)
      BOOST_TEST_EQ(q.pull(), 42);
    BOOST_TEST(q.empty());

  }
  catch (...)
  {
    go.set_value();
    BOOST_TEST(false);
  }
}

void test_concurrent_pull_on_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::promise<void> go;
  boost::shared_future<void> ready(go.get_future());

  boost::promise<void> pull_ready[n];
  boost::future<int> pull_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      q.push(42);

    for (unsigned int i =0; i< n; ++i)
      pull_done[i]=boost::async(boost::launch::async,
#if ! defined BOOST_NO_CXX11_LAMBDAS
        [&q,ready,&pull_ready,i]()
        {
          pull_ready[i].set_value();
          ready.wait();
          return q.pull();
        }
#else
        call_pull(q,ready,pull_ready[i])
#endif
    );

    for (unsigned int i = 0; i < n; ++i)
      pull_ready[i].get_future().wait();
    go.set_value();

    for (unsigned int i = 0; i < n; ++i)
      BOOST_TEST_EQ(pull_done[i].get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    go.set_value();
    BOOST_TEST(false);
  }
}

int main()
{
  test_concurrent_push_and_pull_on_empty_queue();
  test_concurrent_push_on_empty_queue();
  test_concurrent_pull_on_queue();

  return boost::report_errors();
}

