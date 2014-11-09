// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_DETAIL_SCHEDULED_EXECUTOR_BASE_HPP
#define BOOST_THREAD_EXECUTORS_DETAIL_SCHEDULED_EXECUTOR_BASE_HPP

#include <boost/thread/concurrent_queues/sync_timed_queue.hpp>
#include <boost/thread/executors/detail/priority_executor_base.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/thread.hpp>

#include <boost/atomic.hpp>
#include <boost/function.hpp>

namespace boost
{
namespace executors
{
namespace detail
{
  class scheduled_executor_base : public priority_executor_base<concurrent::sync_timed_queue<boost::function<void() > > >
  {
  public:
    typedef boost::function<void()> work;
    //typedef executors::work work;
    typedef chrono::steady_clock clock;
    typedef clock::duration duration;
    typedef clock::time_point time_point;
  protected:

    scheduled_executor_base() {}
  public:

    ~scheduled_executor_base()
    {
      if(!closed())
      {
        this->close();
      }
    }

    void submit_at(work w, const time_point& tp)
    {
      _workq.push(w, tp);
    }

    void submit_after(work w, const duration& dura)
    {
      _workq.push(w, dura+clock::now());
    }

  }; //end class

} //end detail namespace
} //end executors namespace
} //end boost namespace
#endif
