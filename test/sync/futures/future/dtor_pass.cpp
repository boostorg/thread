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

// <boost/thread/future.hpp>

// class promise<R>

// ~promise();

#define BOOST_THREAD_VERSION 2
#include <boost/exception/exception.hpp>

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>
#if defined BOOST_THREAD_FUTURE_USES_ALLOCATORS
#include <libs/thread/test/sync/futures/test_allocator.hpp>
#endif

int main()
{
#if defined BOOST_THREAD_FUTURE_USES_ALLOCATORS
  BOOST_TEST(test_alloc_base::count == 0);
  {
      typedef int T;
      boost::future<T> f;
      {
          boost::promise<T> p(boost::container::allocator_arg, test_allocator<T>());
          BOOST_TEST(test_alloc_base::count == 1);
          f = p.get_future();
          BOOST_TEST(test_alloc_base::count == 1);
          BOOST_TEST(f.valid());
      }
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);
  {
      typedef int& T;
      boost::future<T> f;
      {
          boost::promise<T> p(boost::container::allocator_arg, test_allocator<int>());
          BOOST_TEST(test_alloc_base::count == 1);
          f = p.get_future();
          BOOST_TEST(test_alloc_base::count == 1);
          BOOST_TEST(f.valid());
      }
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);
  {
      typedef void T;
      boost::future<T> f;
      {
          boost::promise<T> p(boost::container::allocator_arg, test_allocator<T>());
          BOOST_TEST(test_alloc_base::count == 1);
          f = p.get_future();
          BOOST_TEST(test_alloc_base::count == 1);
          BOOST_TEST(f.valid());
      }
      BOOST_TEST(test_alloc_base::count == 1);
      BOOST_TEST(f.valid());
  }
  BOOST_TEST(test_alloc_base::count == 0);
#endif

  return boost::report_errors();
}

