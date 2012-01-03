//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/timed_mutex.hpp>

// class timed_mutex;

// void lock();

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::timed_mutex m;

typedef boost::chrono::system_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;

void f()
{
  time_point t0 = Clock::now();
  m.lock();
  time_point t1 = Clock::now();
  m.unlock();
  ns d = t1 - t0 - ms(250);
  BOOST_TEST(d < ns(2500000)); // within 2.5ms
}

int main()
{
  m.lock();
  boost::thread t(f);
  boost::this_thread::sleep_for(ms(250));
  m.unlock();
  t.join();

  return boost::report_errors();
}

