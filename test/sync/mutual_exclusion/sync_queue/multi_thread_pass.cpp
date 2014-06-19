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
#define BOOST_THREAD_QUEUE_DEPRECATE_OLD

#include <boost/thread/sync_queue.hpp>
#include <boost/thread/future.hpp>
#include <boost/thread/barrier.hpp>

#include <boost/detail/lightweight_test.hpp>

template <typename ValueType, void(boost::sync_queue<ValueType>::*push_back)(const ValueType&)>
struct call_push_back
{
  boost::sync_queue<ValueType> *q_;
  boost::barrier *go_;

  call_push_back(boost::sync_queue<ValueType> *q, boost::barrier *go) :
    q_(q), go_(go)
  {
  }
  typedef void result_type;
  void operator()()
  {
    go_->wait();
    (q_->*push_back)(42);
  }
};

template <typename ValueType, boost::queue_op_status(boost::sync_queue<ValueType>::*push_back)(const ValueType&)>
struct call_op_push_back
{
  boost::sync_queue<ValueType> *q_;
  boost::barrier *go_;

  call_op_push_back(boost::sync_queue<ValueType> *q, boost::barrier *go) :
    q_(q), go_(go)
  {
  }
  typedef boost::queue_op_status result_type;
  boost::queue_op_status operator()()
  {
    go_->wait();
    return (q_->*push_back)(42);
  }
};

template <typename ValueType, ValueType(boost::sync_queue<ValueType>::*pull_front)()>
struct call_pull_front
{
  boost::sync_queue<ValueType> *q_;
  boost::barrier *go_;

  call_pull_front(boost::sync_queue<ValueType> *q, boost::barrier *go) :
    q_(q), go_(go)
  {
  }
  typedef ValueType result_type;
  ValueType operator()()
  {
    go_->wait();
    return (q_->*pull_front)();
  }
};

template <typename ValueType, boost::queue_op_status(boost::sync_queue<ValueType>::*pull_front)(ValueType&)>
struct call_op_pull_front
{
  boost::sync_queue<ValueType> *q_;
  boost::barrier *go_;

  call_op_pull_front(boost::sync_queue<ValueType> *q, boost::barrier *go) :
    q_(q), go_(go)
  {
  }
  typedef boost::queue_op_status result_type;
  boost::queue_op_status operator()(ValueType& v)
  {
    go_->wait();
    return (q_->*pull_front)(v);
  }
};

void test_concurrent_push_and_pull_on_empty_queue()
{
  boost::sync_queue<int> q;

  boost::barrier go(2);

  boost::future<void> push_done;
  boost::future<int> pull_done;

  try
  {
    push_done=boost::async(boost::launch::async,
                           call_push_back<int, &boost::sync_queue<int>::push_back>(&q,&go));
    pull_done=boost::async(boost::launch::async,
                           call_pull_front<int, &boost::sync_queue<int>::pull_front>(&q,&go));

    push_done.get();
    BOOST_TEST_EQ(pull_done.get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_push_and_wait_pull_on_empty_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::barrier go(n);

  boost::future<boost::queue_op_status> pull_done[n];
  int results[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      pull_done[i]=boost::async(boost::launch::async,
                                call_op_pull_front<int, &boost::sync_queue<int>::wait_pull_front>(&q,&go),
                                boost::ref(results[i]));

    for (unsigned int i =0; i< n; ++i)
      q.push_back(42);

    for (unsigned int i = 0; i < n; ++i) {
      BOOST_TEST(pull_done[i].get() == boost::queue_op_status::success);
      BOOST_TEST_EQ(results[i], 42);
    }
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_wait_pull_and_close_on_empty_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::barrier go(n);

  boost::future<boost::queue_op_status> pull_done[n];
  int results[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      pull_done[i]=boost::async(boost::launch::async,
                                call_op_pull_front<int, &boost::sync_queue<int>::wait_pull_front>(&q,&go),
                                boost::ref(results[i]));

    q.close();

    for (unsigned int i = 0; i < n; ++i) {
      BOOST_TEST(pull_done[i].get() == boost::queue_op_status::closed);
    }
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_push_on_empty_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::barrier go(n);
  boost::future<void> push_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      push_done[i]=boost::async(boost::launch::async,
                                call_push_back<int, &boost::sync_queue<int>::push_back>(&q,&go));

    for (unsigned int i = 0; i < n; ++i)
      push_done[i].get();

    BOOST_TEST(!q.empty());
    for (unsigned int i =0; i< n; ++i)
      BOOST_TEST_EQ(q.pull_front(), 42);
    BOOST_TEST(q.empty());

  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

void test_concurrent_pull_on_queue()
{
  boost::sync_queue<int> q;
  const unsigned int n = 3;
  boost::barrier go(n);

  boost::future<int> pull_done[n];

  try
  {
    for (unsigned int i =0; i< n; ++i)
      q.push_back(42);

    for (unsigned int i =0; i< n; ++i)
      pull_done[i]=boost::async(boost::launch::async,
                                call_pull_front<int, &boost::sync_queue<int>::pull_front>(&q,&go));

    for (unsigned int i = 0; i < n; ++i)
      BOOST_TEST_EQ(pull_done[i].get(), 42);
    BOOST_TEST(q.empty());
  }
  catch (...)
  {
    BOOST_TEST(false);
  }
}

int main()
{
  test_concurrent_push_and_pull_on_empty_queue();
  test_concurrent_push_on_empty_queue();
  test_concurrent_pull_on_queue();
  test_concurrent_push_and_wait_pull_on_empty_queue();
  test_concurrent_wait_pull_and_close_on_empty_queue();

  return boost::report_errors();
}

