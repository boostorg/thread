// Copyright (C) 2010 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 2

#include <iostream>
#include <boost/thread.hpp>

void thread()
{
  std::cout << "Sleeping for 10 seconds - change time\n";
  boost::this_thread::sleep_for(boost::chrono::seconds(30));
  std::cout << "Ended\n";
  //while (1)     ; // Never quit
}

boost::thread example(thread);

int main()
{
  std::cout << "Main thread START\n";
  boost::this_thread::sleep_for(boost::chrono::seconds(30));
  std::cout << "Main thread END\n";
  //while (1)     ; // Never quit
  example.join();
  return 0;
}

