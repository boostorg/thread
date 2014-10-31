// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SCHEDULING_ADAPTOR_HPP
#define SCHEDULING_ADAPTOR_HPP

#include <boost/thread/executors/detail/scheduled_executor_base.hpp>

namespace boost
{
namespace executors
{

  template <typename Executor>
  class scheduling_adpator : public detail::scheduled_executor_base
  {
  private:
    Executor& _exec;
    thread _scheduler;
  public:

    scheduling_adpator(Executor& ex)
      : super(),
        _exec(ex),
        _scheduler(&scheduling_adpator::scheduler_loop, this) {}

    ~scheduling_adpator()
    {
      this->close();
      _scheduler.join();
    }

    Executor& underlying_executor()
    {
        return _exec;
    }

  private:
    typedef detail::scheduled_executor_base super;
    void scheduler_loop();
  }; //end class

  template<typename Executor>
  void scheduling_adpator<Executor>::scheduler_loop()
  {
    while(!super::_workq.is_closed() || !super::_workq.empty())
    {
      try
      {
        super::work fn = super::_workq.pull();
        _exec.submit(fn);
      }
      catch(std::exception& err)
      {
        // debug std::err << err.what() << std::endl;
        return;
      }
    }
  }

} //end executors

  using executors::scheduling_adpator;

} //end boost
#endif
