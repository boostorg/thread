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

// <boost/thread/locks.hpp>

// template <class Mutex> class unique_lock;

// unique_lock(mutex_type& m, try_to_lock_t);


#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/detail/lightweight_test.hpp>

#if defined BOOST_THREAD_USES_CHRONO

boost::mutex m;

typedef boost::chrono::system_clock Clock;
typedef Clock::time_point time_point;
typedef Clock::duration duration;
typedef boost::chrono::milliseconds ms;
typedef boost::chrono::nanoseconds ns;

void f()
{
  time_point t0 = Clock::now();
  {
    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  {
    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  {
    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
    BOOST_TEST(lk.owns_lock() == false);
  }
  while (true)
  {
    boost::unique_lock<boost::mutex> lk(m, boost::try_to_lock);
    if (lk.owns_lock()) break;
  }
  time_point t1 = Clock::now();
  //m.unlock();
  ns d = t1 - t0 - ms(250);
  // This test is spurious as it depends on the time the thread system switches the threads
  BOOST_TEST(d < ns(50000000)+ms(1000)); // within 50ms
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
#else
#error "Test not applicable: BOOST_THREAD_USES_CHRONO not defined for this platform as not supported"
#endif

