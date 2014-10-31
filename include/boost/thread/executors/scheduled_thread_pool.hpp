// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SCHEDULED_THREAD_POOL_HPP
#define SCHEDULED_THREAD_POOL_HPP

#include <boost/thread/executors/detail/scheduled_executor_base.hpp>

namespace boost
{
namespace executors
{

  class scheduled_thread_pool : public detail::scheduled_executor_base
  {
  private:
    thread_group _workers;
  public:

    scheduled_thread_pool(size_t num_threads) : super()
    {
      for(size_t i = 0; i < num_threads; i++)
      {
        _workers.create_thread(bind(&scheduled_thread_pool::worker_loop, this));
      }
    }

    ~scheduled_thread_pool()
    {
      this->close();
      _workers.join_all();
    }

  private:
    typedef detail::scheduled_executor_base super;
    inline void worker_loop();
  }; //end class

  void scheduled_thread_pool::worker_loop()
  {
    while(!super::_workq.is_closed() || !super::_workq.empty())
    {
      try
      {
        super::work fn = super::_workq.pull();
        fn();
      }
      catch(std::exception& err)
      {
        // debug std::err << err.what() << std::endl;
        return;
      }
    }
  }

} //end executors namespace

using executors::scheduled_thread_pool;

} //end boost
#endif

