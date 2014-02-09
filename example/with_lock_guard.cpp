// (C) Copyright 2013 Ruslan Baratov
// Copyright (C) 2014 Vicente Botet
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See www.boost.org/libs/thread for documentation.

#define BOOST_THREAD_VERSION 4

#include <iostream> // std::cout
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/with_lock_guard.hpp>

boost::mutex m; // protection for 'x' and 'std::cout'
int x;

#if defined(BOOST_NO_CXX11_LAMBDAS)
void print_x() {
  ++x;
  std::cout << "x = " << x << std::endl;
}

void job() {
  for (int i = 0; i < 10; ++i) {
    boost::with_lock_guard(m, print_x);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
}
#else
void job() {
  for (int i = 0; i < 10; ++i) {
    boost::with_lock_guard(
        m,
        []() {
          ++x;
          std::cout << "x = " << x << std::endl;
        }
    );
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }
}
#endif

int main() {
#if defined(BOOST_NO_CXX11_LAMBDAS)
  std::cout << "(no lambdas)" << std::endl;
#endif
  boost::scoped_thread<> thread_1(job);
  boost::scoped_thread<> thread_2(job);
  boost::scoped_thread<> thread_3(job);
  return 0;
}
