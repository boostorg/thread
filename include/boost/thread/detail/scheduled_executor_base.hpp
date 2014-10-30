// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SCHEDULED_EXECUTOR_HPP
#define SCHEDULED_EXECUTOR_HPP

#include <boost/atomic.hpp>
#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/concurrent_queues/sync_timed_queue.hpp>

namespace boost
{
namespace detail
{
  class scheduled_executor_base
  {
  public:
    typedef boost::function<void()> work;
    typedef chrono::steady_clock clock;
    typedef clock::duration duration;
    typedef clock::time_point time_point;
  protected:
    sync_timed_queue<work> _workq;

    scheduled_executor_base() {}
  public:

    ~scheduled_executor_base() //virtual?
    {
      if(!_workq.is_closed())
      {
        this->close();
      }
    }

    void close()
    {
      _workq.close();
    }

    void submit(work w)
    {
      _workq.push(w, clock::now());
    }

    void submit_at(work w, const time_point& tp)
    {
      _workq.push(w, tp);
    }

    void submit_after(work w, const duration& dura)
    {
      _workq.push(w, dura);
    }
  }; //end class
} //end detail namespace
} //end boost namespace
#endif
