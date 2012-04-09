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

// <boost/thread/shared_lock_guard.hpp>

// template <class Mutex> class shared_lock_guard;

// shared_lock_guard(shared_lock_guard const&) = delete;


#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::shared_mutex m0;
boost::shared_mutex m1;

int main()
{
  boost::shared_lock_guard<boost::shared_mutex> lk0(m0);
  boost::shared_lock_guard<boost::shared_mutex> lk1 = lk0;
}

void remove_unused_warning()
{
  //../../../boost/system/error_code.hpp:214:36: warning: �boost::system::posix_category� defined but not used [-Wunused-variable]
  //../../../boost/system/error_code.hpp:215:36: warning: �boost::system::errno_ecat� defined but not used [-Wunused-variable]
  //../../../boost/system/error_code.hpp:216:36: warning: �boost::system::native_ecat� defined but not used [-Wunused-variable]

  (void)boost::system::posix_category;
  (void)boost::system::errno_ecat;
  (void)boost::system::native_ecat;
}
