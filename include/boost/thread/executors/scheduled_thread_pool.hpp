// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULED_THREAD_POOL_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULED_THREAD_POOL_HPP

#include <boost/thread/executors/detail/scheduled_executor_base.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/csbl/vector.hpp>

namespace boost
{
namespace executors
{

  class scheduled_thread_pool : public detail::scheduled_executor_base<>
  {
  private:
    typedef scoped_thread<> thread_t;
    typedef csbl::vector<thread_t> thread_vector;
    thread_vector threads;

  public:

    scheduled_thread_pool(unsigned const thread_count = thread::hardware_concurrency()+1) : super()
    {

      try
      {
        threads.reserve(thread_count);
        for (unsigned i = 0; i < thread_count; ++i)
        {
#if 1
          thread th (&super::loop, this);
          threads.push_back(thread_t(boost::move(th)));
#else
          threads.push_back(thread_t(&super::loop, this)); // do not compile
#endif
        }
      }
      catch (...)
      {
        close();
        throw;
      }
    }

    ~scheduled_thread_pool()
    {
      this->close();
    }

  private:
    typedef detail::scheduled_executor_base<> super;
  }; //end class

} //end executors namespace

using executors::scheduled_thread_pool;

} //end boost
#endif

