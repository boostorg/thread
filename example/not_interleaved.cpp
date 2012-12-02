// (C) Copyright 2012 Howard Hinnant
// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// adapted from the example given by Howard Hinnant in


#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/thread/externally_locked_stream.hpp>

void use_cerr(boost::externally_locked_stream<std::ostream> &mcerr)
{
  using namespace boost;
  auto tf = chrono::steady_clock::now() + chrono::seconds(10);
  while (chrono::steady_clock::now() < tf)
  {
    mcerr << "logging data to cerr\n";
    this_thread::sleep_for(milliseconds(500));
  }
}

int main()
{
  using namespace boost;

  externally_locked_stream<std::ostream> mcerr(std::cerr, terminal_mutex());
  externally_locked_stream<std::ostream> mcout(std::cerr, terminal_mutex());
  externally_locked_stream<std::istream> mcin(std::cerr, terminal_mutex());

  thread t1(use_cerr, mcerr);
  this_thread::sleep_for(boost::chrono::seconds(2));
  std::string nm;
  mcout << "Enter name: ";
  mcin >> nm;
  t1.join();
  mcout << nm << '\n';
  return 0;
}

