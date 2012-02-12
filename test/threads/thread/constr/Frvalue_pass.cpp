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
#ifndef BOOST_NO_RVALUE_REFERENCES
  MoveOnly(const MoveOnly&);
#else
  MoveOnly(MoveOnly&);
  MoveOnly& operator=(MoveOnly&);
#endif
public:
  typedef int boost_move_emulation_t;
  MoveOnly()
  {
  }
#ifndef BOOST_NO_RVALUE_REFERENCES
  MoveOnly(MoveOnly&&)
  {}
#else
#if defined BOOST_THREAD_USES_MOVE
  MoveOnly(boost::rv<MoveOnly>&)
  {}
  MoveOnly& operator=(boost::rv<MoveOnly>&)
  {
    return *this;
  }
  operator ::boost::rv<MoveOnly>&()
  {
    return *static_cast< ::boost::rv<MoveOnly>* >(this);
  }
  operator const ::boost::rv<MoveOnly>&() const
  {
    return *static_cast<const ::boost::rv<MoveOnly>* >(this);
  }
#else
  MoveOnly(detail::thread_move_t<MoveOnly>)
  {}
#endif
#endif

  void operator()()
  {
  }
};

MoveOnly MakeMoveOnly() {
  MoveOnly x;
  return boost::move(x);
}
int main()
{
  {
    // FIXME The following fails
    boost::thread t = boost::thread( MoveOnly() );
    //boost::thread t = boost::thread( MakeMoveOnly() );
    //boost::thread t (( boost::move( MoveOnly() ) ));
    t.join();
  }
  return boost::report_errors();
}
