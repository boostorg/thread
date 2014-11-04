// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULER_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULER_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/executors/detail/scheduled_executor_base.hpp>

#include <boost/chrono/system_clocks.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace executors
  {
    template <class Executor, class Function>
    class resubmiter
    {
    public:
      resubmiter(Executor& ex, Function funct) :
        ex(ex),
        funct(boost::move(funct))
      {}

      void operator()()
      {
        ex.submit(funct);
      }

    private:
      Executor&   ex;
      Function funct;
    };

    template <class Executor, class Function>
    resubmiter<Executor, typename decay<Function>::type>
    resubmit(Executor& ex, BOOST_THREAD_FWD_REF(Function) funct) {
      return resubmiter<Executor, typename decay<Function>::type >(ex, boost::move(funct));
    }

    template <class Scheduler, class Executor>
    class scheduled_executor_time_point_wrapper
    {
    public:
      typedef chrono::steady_clock clock;

      scheduled_executor_time_point_wrapper(Scheduler& sch, Executor& ex, clock::time_point const& tp) :
        sch(sch),
        ex(ex),
        tp(tp),
        is_closed(false)
      {
      }

      template <class Work>
      void submit(BOOST_THREAD_FWD_REF(Work) w)
      {
        if (closed())
        {
          BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
        }
        sch.submit_at(resubmit(ex,boost::forward<Work>(w)), tp);
      }

      Executor& underlying_executor()
      {
          return ex;
      }
      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      void close()
      {
        is_closed = true;
      }

      bool closed()
      {
        return is_closed || sch.closed() || ex.closed();
      }

    private:
      Scheduler&  sch;
      Executor&   ex;
      clock::time_point  tp;
      bool  is_closed;
    };


    template <class Scheduler, class Executor>
    class scheduled_executor_wrapper
    {
    public:
      typedef chrono::steady_clock clock;
      typedef scheduled_executor_time_point_wrapper<Scheduler, Executor> the_executor;

      scheduled_executor_wrapper(Scheduler& sch, Executor& ex) :
          sch(sch),
          ex(ex),
          is_closed(false)
      {}

      ~scheduled_executor_wrapper()
      {
      }

      Executor& underlying_executor()
      {
          return ex;
      }
      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      void close()
      {
        is_closed = true;
      }

      bool closed()
      {
        return is_closed || ex.closed();
      }

      template <class Work>
      void submit(BOOST_THREAD_FWD_REF(Work) w)
      {
        if (closed())
        {
          BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
        }
        ex.submit(boost::forward<Work>(w));
      }

      template <class Duration>
      the_executor after(Duration const& rel_time)
      {
        return at(clock::now() + rel_time );
      }

      the_executor at(clock::time_point const& abs_time)
      {
        return the_executor(sch, ex, abs_time);
      }

    private:
      Scheduler& sch;
      Executor& ex;
      bool  is_closed;
    }; //end class

    template <class Scheduler>
    class scheduled_time_point_wrapper
    {
    public:
      typedef chrono::steady_clock clock;

      scheduled_time_point_wrapper(Scheduler& sch, clock::time_point const& tp) :
          sch(sch),
          tp(tp),
          is_closed(false)
      {}

      ~scheduled_time_point_wrapper()
      {
      }

      Scheduler& underlying_scheduler()
      {
          return sch;
      }

      void close()
      {
        is_closed = true;
      }

      bool closed()
      {
        return is_closed || sch.closed();
      }

      template <class Work>
      void submit(BOOST_THREAD_FWD_REF(Work) w)
      {
        if (closed())
        {
          BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
        }
        sch.submit_at(boost::forward<Work>(w), tp);
      }

      template <class Executor>
      scheduled_executor_time_point_wrapper<Scheduler, Executor> on(Executor& ex)
      {
        return scheduled_executor_time_point_wrapper<Scheduler, Executor>(sch, ex, tp);
      }

    private:
      Scheduler& sch;
      clock::time_point  tp;
      bool  is_closed;
    }; //end class

    class scheduler : public detail::scheduled_executor_base
    {
    public:
      typedef chrono::steady_clock clock;
      typedef clock::time_point time_point;

      scheduler()
        : super(),
          thr(&super::loop, this) {}

      ~scheduler()
      {
        this->close();
        thr.join();
      }
      template <class Ex>
      scheduled_executor_wrapper<scheduler, Ex> on(Ex& ex)
      {
        return scheduled_executor_wrapper<scheduler, Ex>(*this, ex);
      }

      template <class Duration>
      scheduled_time_point_wrapper<scheduler> after(Duration const& rel_time)
      {
        return at(rel_time + clock::now());
      }

      scheduled_time_point_wrapper<scheduler> at(time_point const& tp)
      {
        return scheduled_time_point_wrapper<scheduler>(*this, tp);
      }

    private:
      typedef detail::scheduled_executor_base super;
      thread thr;
    };


  }
  using executors::resubmiter;
  using executors::resubmit;
  using executors::scheduled_executor_time_point_wrapper;
  using executors::scheduled_executor_wrapper;
  using executors::scheduled_time_point_wrapper;
  using executors::scheduler;
}

#include <boost/config/abi_suffix.hpp>

#endif
