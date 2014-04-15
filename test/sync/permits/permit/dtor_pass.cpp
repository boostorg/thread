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

// <boost/thread/permit>

// class permit;

// permit(const permit&) = delete;

#include <boost/thread/permit.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/detail/lightweight_test.hpp>

boost::permit_c* cv;
boost::mutex m;
typedef boost::unique_lock<boost::mutex> Lock;

bool f_ready = false;
bool g_ready = false;

void f()
{
  Lock lk(m);
  f_ready = true;
  cv->notify_one();
  cv->wait(lk);
  delete cv;
}

void g()
{
  Lock lk(m);
  g_ready = true;
  cv->notify_one();
  while (!f_ready)
  {
    cv->wait(lk);
  }
  cv->notify_one();
}

int main()
{
  cv = new boost::permit_c;
  boost::thread th2(g);
  Lock lk(m);
  while (!g_ready)
  {
    cv->wait(lk);
  }
  lk.unlock();
  boost::thread th1(f);
  th1.join();
  th2.join();
  return boost::report_errors();
}

