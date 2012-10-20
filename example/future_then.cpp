// Copyright (C) 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_DONT_PROVIDE_FUTURE_INVALID_AFTER_GET

#include <boost/thread/future.hpp>
#include <iostream>
#include <string>
#if defined BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION

int p1()
{
  return 123;
}

int p2(boost::future<int>& f)
{
  return 2 * f.get();
}

int main()
{
  boost::future<int> f1 = boost::async(p1);
  boost::future<int> f2 = f1.then(p2);
  std::cout << f2.get() << std::endl;
  return 0;
}
#else

int main()
{
  return 0;
}
#endif
