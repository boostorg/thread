#ifndef BOOST_THREAD_PTHREAD_TIMESPEC_HPP
#define BOOST_THREAD_PTHREAD_TIMESPEC_HPP
//  (C) Copyright 2007-8 Anthony Williams
//  (C) Copyright 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/thread/thread_time.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/date_time/posix_time/conversion.hpp>
#endif
#include <pthread.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/duration.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_THREAD_TIMESPEC_MAC_API
#include <sys/time.h> //for gettimeofday and timeval
#else
#include <time.h>  // for clock_gettime
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail
  {
#if defined BOOST_THREAD_USES_DATETIME
    inline struct timespec to_timespec(boost::posix_time::time_duration const& rel_time)
    {
      struct timespec timeout = { 0,0};
      timeout.tv_sec=rel_time.total_seconds();
      timeout.tv_nsec=(long)(rel_time.fractional_seconds()*(1000000000l/rel_time.ticks_per_second()));
      return timeout;
    }
    inline struct timespec to_timespec(boost::system_time const& abs_time)
    {
      return to_timespec(abs_time-boost::posix_time::from_time_t(0));
    }
#endif
#if defined BOOST_THREAD_USES_CHRONO
    template <class Rep, class Period>
    timespec to_timespec(chrono::duration<Rep, Period> const& rel_time)
    {
      chrono::nanoseconds ns = chrono::ceil<chrono::nanoseconds>(rel_time);
      struct timespec ts;
      ts.tv_sec = static_cast<long>(chrono::duration_cast<chrono::seconds>(ns).count());
      ts.tv_nsec = static_cast<long>((ns - chrono::duration_cast<chrono::seconds>(ns)).count());
      return ts;
    }
    template <class Clock, class Duration>
    timespec to_timespec(chrono::time_point<Clock, Duration> const& t)
    {
      return to_timespec(t.time_since_epoch());
    }

#endif

    inline timespec to_timespec(boost::intmax_t const& ns)
    {
      boost::intmax_t s = ns / 1000000000l;
      struct timespec ts;
      ts.tv_sec = static_cast<long> (s);
      ts.tv_nsec = static_cast<long> (ns - s * 1000000000l);
      return ts;
    }
    inline boost::intmax_t to_nanoseconds_int_max(timespec const& ts)
    {
      return static_cast<boost::intmax_t>(ts.tv_sec) * 1000000000l + ts.tv_nsec;
    }
    inline bool timespec_ge_zero(timespec const& ts)
    {
      return (ts.tv_sec >= 0) || (ts.tv_nsec >= 0);
    }
#if defined BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC

    inline timespec timespec_now_monotonic()
    {
      timespec ts;

      if ( ::clock_gettime( CLOCK_MONOTONIC, &ts ) )
      {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        BOOST_ASSERT(0 && "Boost::Thread - Internal Error");
      }
      return ts;
    }
#endif

    inline timespec timespec_now_realtime()
    {
      timespec ts;

#if defined(BOOST_THREAD_TIMESPEC_MAC_API)
      timeval tv;
      ::gettimeofday(&tv, 0);
      ts.tv_sec = tv.tv_sec;
      ts.tv_nsec = tv.tv_usec * 1000;
#else
      if ( ::clock_gettime( CLOCK_REALTIME, &ts ) )
      {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        BOOST_ASSERT(0 && "Boost::Thread - Internal Error");
      }
#endif
      return ts;
    }
    inline timespec timespec_zero()
    {
      timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = 0;
      return ts;
    }
    inline timespec timespec_plus(timespec const& lhs, timespec const& rhs)
    {
      return to_timespec(to_nanoseconds_int_max(lhs) + to_nanoseconds_int_max(rhs));
    }
    inline timespec timespec_minus(timespec const& lhs, timespec const& rhs)
    {
      return to_timespec(to_nanoseconds_int_max(lhs) - to_nanoseconds_int_max(rhs));
    }
    inline bool timespec_gt(timespec const& lhs, timespec const& rhs)
    {
      return to_nanoseconds_int_max(lhs) > to_nanoseconds_int_max(rhs);
    }
    inline bool timespec_ge(timespec const& lhs, timespec const& rhs)
    {
      return to_nanoseconds_int_max(lhs) >= to_nanoseconds_int_max(rhs);
    }

    inline timespec real_to_abs_internal_timespec(timespec const& abs_time)
    {
#ifdef BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
      struct timespec const m_now = boost::detail::timespec_now_monotonic();
      struct timespec const r_now = boost::detail::timespec_now_realtime();
      struct timespec const diff = boost::detail::timespec_minus(abs_time, r_now);
      return boost::detail::timespec_plus(m_now, diff);
#else
      return abs_time;
#endif
    }

    inline timespec duration_to_abs_internal_timespec(timespec const& rel_time)
    {
#ifdef BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
      struct timespec const now = boost::detail::timespec_now_monotonic();
#else
      struct timespec const now = boost::detail::timespec_now_realtime();
#endif
      return boost::detail::timespec_plus(now, rel_time);
    }

#if defined BOOST_THREAD_USES_DATETIME
    inline timespec to_abs_internal_timespec(boost::system_time const& abs_time)
    {
      return real_to_abs_internal_timespec(to_timespec(abs_time));
    }

    inline timespec to_abs_internal_timespec(boost::posix_time::time_duration const& rel_time)
    {
      return duration_to_abs_internal_timespec(to_timespec(rel_time));
    }
#endif

#if defined BOOST_THREAD_USES_CHRONO
#if defined(BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC)
    typedef chrono::steady_clock internal_clock_t;
#else
    typedef chrono::system_clock internal_clock_t;
#endif

    template <class Duration>
    timespec to_abs_internal_timespec(chrono::time_point<internal_clock_t, Duration> const& t)
    {
      return to_timespec(t);
    }

    template <class Clock, class Duration>
    timespec to_abs_internal_timespec(chrono::time_point<Clock, Duration> const& t)
    {
      internal_clock_t::time_point s_now = internal_clock_t::now();
      typename Clock::time_point c_now = Clock::now();
      return to_timespec(s_now + chrono::ceil<chrono::nanoseconds>(t - c_now));
    }

    template <class Rep, class Period>
    timespec to_abs_internal_timespec(chrono::duration<Rep, Period> const& d)
    {
      return to_timespec(internal_clock_t::now() + d);
    }
#endif

    // TODO: This needs to be moved to some place more appropriate.
    inline int cond_init(pthread_cond_t& cond)
    {
#ifdef BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
      pthread_condattr_t attr;
      int res = pthread_condattr_init(&attr);
      if (res)
      {
        return res;
      }
      pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
      res=pthread_cond_init(&cond,&attr);
      pthread_condattr_destroy(&attr);
      return res;
#else
      return pthread_cond_init(&cond,NULL);
#endif
    }

  }
}

#include <boost/config/abi_suffix.hpp>

#endif
