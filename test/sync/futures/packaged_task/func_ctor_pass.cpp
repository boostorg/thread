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
// class packaged_task<R>

// template <class F>
//     explicit packaged_task(F&& f);


#define BOOST_THREAD_VERSION 2
#include <boost/thread/future.hpp>
#include <boost/detail/lightweight_test.hpp>

double fct()
{
  return 5.0;
}
long lfct()
{
  return 5;
}

class A
{
    long data_;

public:
    static int n_moves;
    static int n_copies;

    explicit A(long i) : data_(i) {}
#ifndef BOOST_NO_RVALUE_REFERENCES
  A(A&& a) : data_(a.data_)
  {
    ++n_moves; a.data_ = -1;
  }
#else

#if defined BOOST_THREAD_USES_MOVE
  operator ::boost::rv<A>&()
  {
    return *static_cast< ::boost::rv<A>* >(this);
  }
  operator const ::boost::rv<A>&() const
  {
    return *static_cast<const ::boost::rv<A>* >(this);
  }
  ::boost::rv<A>& move()
  {
    return *static_cast< ::boost::rv<A>* >(this);
  }
  const ::boost::rv<A>& move() const
  {
    return *static_cast<const ::boost::rv<A>* >(this);
  }

  A(boost::rv<A>& a) : data_(a.data_)
  {
    ++n_moves; a.data_ = -1;
  }
#else
  operator boost::detail::thread_move_t<A>()
  {
      return boost::detail::thread_move_t<A>(*this);
  }
  boost::detail::thread_move_t<A> move()
  {
      return boost::detail::thread_move_t<A>(*this);
  }
  A(boost::detail::thread_move_t<A> a) : data_(a.data_)
  {
    ++n_moves; a.data_ = -1;
  }

#endif
#endif
  A(const A& a) : data_(a.data_) {++n_copies;}

    long operator()() const {return data_;}
    long operator()(long i, long j) const {return data_ + i + j;}
};

int A::n_moves = 0;
int A::n_copies = 0;


int main()
{
  {
      boost::packaged_task<double> p(BOOST_EXPLICIT_MOVE(A(5)));
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_EXPLICIT_MOVE(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST(A::n_copies == 0);
      BOOST_TEST(A::n_moves > 0);
  }
  A::n_copies = 0;
  A::n_copies = 0;
  {
      A a(5);
      boost::packaged_task<double> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_EXPLICIT_MOVE(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST(A::n_copies > 0);
      BOOST_TEST(A::n_moves > 0);
  }

  A::n_copies = 0;
  A::n_copies = 0;
  {
      const A a(5);
      boost::packaged_task<double> p(a);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_EXPLICIT_MOVE(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
      BOOST_TEST(A::n_copies > 0);
      BOOST_TEST(A::n_moves > 0);
  }
  {
      boost::packaged_task<double> p(fct);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_EXPLICIT_MOVE(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
  }
  {
      boost::packaged_task<double> p(&lfct);
      BOOST_TEST(p.valid());
      boost::future<double> f = BOOST_EXPLICIT_MOVE(p.get_future());
      //p(3, 'a');
      p();
      BOOST_TEST(f.get() == 5.0);
  }

  return boost::report_errors();
}

