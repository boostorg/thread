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
#ifndef BOOST_NO_DELETED_FUNCTIONS
  public:
  MoveOnly(const MoveOnly&)=delete;
  MoveOnly& operator=(MoveOnly const&);
#else
  private:
  MoveOnly(MoveOnly&);
  MoveOnly& operator=(MoveOnly&);
  public:
#endif
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
  ::boost::rv<MoveOnly>& move()
  {
    return *static_cast< ::boost::rv<MoveOnly>* >(this);
  }
  const ::boost::rv<MoveOnly>& move() const
  {
    return *static_cast<const ::boost::rv<MoveOnly>* >(this);
  }
#else
#error
  MoveOnly(detail::thread_move_t<MoveOnly>)
  {}
#endif
#endif

  void operator()()
  {
  }
};


int main()
{
  {
    boost::thread t = boost::thread( BOOST_EXPLICIT_MOVE(MoveOnly()), BOOST_EXPLICIT_MOVE(MoveOnly()) );
    t.join();
  }
  return boost::report_errors();
}
