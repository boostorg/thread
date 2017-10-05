// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2011 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_V2_THREAD_HPP
#define BOOST_THREAD_V2_THREAD_HPP

#include <boost/thread/detail/config.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_types.hpp>

namespace boost
{
  namespace this_thread
  {
    namespace no_interruption_point
    {
#ifdef BOOST_THREAD_USES_CHRONO

// Use pthread_delay_np or nanosleep whenever possible here in the no_interruption_point
// namespace because they do not provide an interruption point.
#if defined BOOST_THREAD_SLEEP_FOR_IS_STEADY

    // sleep_for(const chrono::duration<Rep, Period>& d) is defined in pthread/thread_data.hpp

    template <class Duration>
    void sleep_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
    {
      sleep_for(t - chrono::steady_clock::now());
    }

    template <class Clock, class Duration>
    void sleep_until(const chrono::time_point<Clock, Duration>& t)
    {
      typedef typename common_type<Duration, typename Clock::duration>::type CD;
      CD d = t - Clock::now();
      while (d > CD::zero())
      {
        d = (std::min)(d, CD(chrono::milliseconds(100)));
        sleep_for(d);
        d = t - Clock::now();
      }
    }

#else

    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      if (d > chrono::duration<Rep, Period>::zero())
      {
        sleep_until(chrono::steady_clock::now() + d);
      }
    }

    template <class Duration>
    void sleep_until(const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
    {
      mutex mut;
      condition_variable cv;
      unique_lock<mutex> lk(mut);
      while (cv_status::no_timeout == cv.wait_until(lk, t)) {}
    }

    template <class Clock, class Duration>
    void sleep_until(const chrono::time_point<Clock, Duration>& t)
    {
      typedef typename common_type<Duration, typename Clock::duration>::type CD;
      CD d = t - Clock::now();
      while (d > CD::zero())
      {
        d = (std::min)(d, CD(chrono::milliseconds(100)));
        sleep_until(detail::internal_chrono_clock::now() + d);
        d = t - Clock::now();
      }
    }

#endif

#endif
    }
#ifdef BOOST_THREAD_USES_CHRONO

    template <class Duration>
    void sleep_until(const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
    {
      mutex mut;
      condition_variable cv;
      unique_lock<mutex> lk(mut);
      while (cv_status::no_timeout == cv.wait_until(lk, t)) {}
    }

    template <class Clock, class Duration>
    void sleep_until(const chrono::time_point<Clock, Duration>& t)
    {
      typedef typename common_type<Duration, typename Clock::duration>::type CD;
      CD d = t - Clock::now();
      while (d > CD::zero())
      {
        d = (std::min)(d, CD(chrono::milliseconds(100)));
        sleep_until(detail::internal_chrono_clock::now() + d);
        d = t - Clock::now();
      }
    }

    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      if (d > chrono::duration<Rep, Period>::zero())
      {
        sleep_until(chrono::steady_clock::now() + d);
      }
    }

#endif
  }
}


#endif
