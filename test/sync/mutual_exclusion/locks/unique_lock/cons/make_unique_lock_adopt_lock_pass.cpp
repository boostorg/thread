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

// <boost/thread/lock_factories.hpp>

// template <class Mutex> class unique_lock;
// unique_lock<Mutex> make_unique_lock(Mutex&, adopt_lock_t);

#include <boost/thread/lock_factories.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

#if ! defined(BOOST_NO_CXX11_AUTO) && ! defined BOOST_NO_CXX11_HDR_INITIALIZER_LIST

int main()
{
  boost::mutex m;
  m.lock();
  auto lk = boost::make_unique_lock(m, boost::adopt_lock);
  BOOST_TEST(lk.mutex() == &m);
  BOOST_TEST(lk.owns_lock() == true);

  return boost::report_errors();
}

#else
int main()
{
  return boost::report_errors();
}
#endif

