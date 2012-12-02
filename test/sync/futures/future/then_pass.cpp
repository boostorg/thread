// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/future.hpp>

// class future<R>

// template<typename F>
// auto then(F&& func) -> BOOST_THREAD_FUTURE<decltype(func(*this))>;

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_DONT_PROVIDE_FUTURE_INVALID_AFTER_GET

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

int p1()
{
  return 1;
}

int p2(boost::future<int>& f)
{
  return 2 * f.get();
}

int main()
{
  {
    boost::future<int> f1 = boost::async(p1);
    boost::future<int> f2 = f1.then(p2);
    BOOST_TEST(f2.get()==2);
  }
  {
    boost::future<int> f2 = boost::async(p1).then(p2);
    BOOST_TEST(f2.get()==2);
  }
  {
    boost::future<int> f1 = boost::async(p1);
    boost::future<int> f2 = f1.then(p2).then(p2);
    BOOST_TEST(f2.get()==4);
  }
  {
    boost::future<int> f2 = boost::async(p1).then(p2).then(p2);
    BOOST_TEST(f2.get()==4);
  }

  return boost::report_errors();
}

#else

int main()
{
  return 0;
}
#endif
