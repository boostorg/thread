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

// <boost/thread/thread.hpp>

// class thread

// template <class F, class ...Args> thread(F&& f, Args&&... args);

#include <boost/thread/thread.hpp>
#include <new>
#include <cstdlib>
#include <cassert>
#include <boost/detail/lightweight_test.hpp>

class MoveOnly
{
  MoveOnly(const MoveOnly&);
public:
  MoveOnly()
  {
  }
#ifndef BOOST_NO_RVALUE_REFERENCES
  MoveOnly(MoveOnly&&)
  {}
  void operator()(MoveOnly&&)
  {
  }
#else
  MoveOnly(detail::thread_move_t<MoveOnly>)
  {}
  void operator()(detail::thread_move_t<MoveOnly>)
  {
  }
#endif

};

int main()
{
  {
    boost::thread t = boost::thread(MoveOnly(), MoveOnly());
    t.join();
  }
  return boost::report_errors();
}
