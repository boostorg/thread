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

// <boost/thread/mutex.hpp>

// <mutex>

// struct defer_lock_t {};
// struct try_to_lock_t {};
// struct adopt_lock_t {};
//
// constexpr defer_lock_t  defer_lock{};
// constexpr try_to_lock_t try_to_lock{};
// constexpr adopt_lock_t  adopt_lock{};

#include <boost/thread/mutex.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  typedef boost::defer_lock_t T1;
  typedef boost::try_to_lock_t T2;
  typedef boost::adopt_lock_t T3;

  T1 t1 = boost::defer_lock;
  T2 t2 = boost::try_to_lock;
  T3 t3 = boost::adopt_lock;

  return boost::report_errors();
}

