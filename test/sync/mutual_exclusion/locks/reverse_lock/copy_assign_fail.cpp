// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/locks.hpp>

// template <class Mutex> class unlock_guard;

// unlock_guard& operator=(unlock_guard const&) = delete;

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>


int main()
{
  boost::mutex m0;
  boost::mutex m1;
  boost::unique_lock<boost::mutex> lk0(m0);
  boost::unique_lock<boost::mutex> lk1(m1);
  {
    boost::unlock_guard<boost::unique_lock<boost::mutex> > lg0(lk0);
    boost::unlock_guard<boost::unique_lock<boost::mutex> > lg1(lk1);
    lk1 = lk0;
  }

}

