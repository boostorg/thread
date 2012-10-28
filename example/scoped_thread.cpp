// (C) Copyright 2009-2012 Anthony Williams
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>
//#include <string>
#include <boost/thread/scoped_thread.hpp>

void do_something(int& i)
{
  ++i;
}

struct func
{
  int& i;

  func(int& i_) :
    i(i_)
  {
  }

  void operator()()
  {
    for (unsigned j = 0; j < 1000000; ++j)
    {
      do_something(i);
    }
  }
};

void do_something_in_current_thread()
{
}


int main()
{
  {
    int some_local_state;
    boost::strict_scoped_thread<> t( (boost::thread(func(some_local_state))));

    do_something_in_current_thread();
  }
  {
    int some_local_state;
    boost::thread t(( func(some_local_state) ));
    boost::strict_scoped_thread<> g( (boost::move(t)) );

    do_something_in_current_thread();
  }
  return 0;
}

