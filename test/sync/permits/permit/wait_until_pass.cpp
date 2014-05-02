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

// <boost/thread/permit>

// class permit;

// permit(const permit&) = delete;

#include <boost/thread/permit.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

struct Clock
{
  typedef boost::chrono::milliseconds duration;
  typedef duration::rep rep;
  typedef duration::period period;
  typedef boost::chrono::time_point<Clock> time_point;
  static const bool is_steady = true;

  static time_point now()
  {
    using namespace boost::chrono;
    return time_point(duration_cast<duration> (steady_clock::now().time_since_epoch()));
  }
};

boost::permit_c cv;
boost::mutex mut;

int test1 = 0;
int test2 = 0;

int runs = 0;

void f()
{
  boost::unique_lock < boost::mutex > lk(mut);
  BOOST_TEST(test2 == 0);
  test1 = 1;
  cv.notify_one();
  Clock::time_point t0 = Clock::now();
  Clock::time_point t = t0 + Clock::duration(250);
  int count=0;
  while (test2 == 0 && cv.wait_until(lk, t) == boost::cv_status::no_timeout)
    count++;
  Clock::time_point t1 = Clock::now();
  if (runs == 0)
  {
    BOOST_TEST(t1 - t0 < Clock::duration(250));
    BOOST_TEST(test2 != 0);
  }
  else
  {
    // This test is spurious as it depends on the time the thread system switches the threads
    BOOST_TEST(t1 - t0 - Clock::duration(250) < Clock::duration(count*250+5+1000));
    BOOST_TEST(test2 == 0);
  }
  ++runs;
}

int main()
{
  {
    boost::unique_lock < boost::mutex > lk(mut);
    boost::thread t(f);
    BOOST_TEST(test1 == 0);
    while (test1 == 0)
      cv.wait(lk);
    BOOST_TEST(test1 != 0);
    test2 = 1;
    lk.unlock();
    cv.notify_one();
    t.join();
  }
  test1 = 0;
  test2 = 0;
  {
    boost::unique_lock < boost::mutex > lk(mut);
    boost::thread t(f);
    BOOST_TEST(test1 == 0);
    while (test1 == 0)
      cv.wait(lk);
    BOOST_TEST(test1 != 0);
    lk.unlock();
    t.join();
  }

  return boost::report_errors();
}

#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif