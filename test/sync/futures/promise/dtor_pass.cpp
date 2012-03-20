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

#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

int main()
{
  std::cout << __LINE__ << std::endl;
  {
      typedef int T;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
          p.set_value(3);
      }
      BOOST_TEST(f.get() == 3);
  }
  std::cout << __LINE__ << std::endl;
  {
      typedef int T;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
      }
      try
      {
          T i = f.get();
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::broken_promise));
      }
  }

  std::cout << __LINE__ << std::endl;
  {
      typedef int& T;
      int i = 4;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
          p.set_value(i);
      }
      BOOST_TEST(&f.get() == &i);
  }
  std::cout << __LINE__ << std::endl;
  {
      typedef int& T;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
      }
      try
      {
          T i = f.get();
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::broken_promise));
      }
  }

  std::cout << __LINE__ << std::endl;
  {
      typedef void T;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
          p.set_value();
      }
      f.get();
      BOOST_TEST(true);
  }
  std::cout << __LINE__ << std::endl;
  {
      typedef void T;
      boost::future<T> f;
      {
          boost::promise<T> p;
          f = p.get_future();
      }
      try
      {
          f.get();
          BOOST_TEST(false);
      }
      catch (const boost::future_error& e)
      {
          BOOST_TEST(e.code() == boost::system::make_error_code(boost::future_errc::broken_promise));
      }
  }

  return boost::report_errors();
}

