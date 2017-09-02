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
#include <boost/chrono/system_clocks.hpp>
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

    inline timespec int_to_timespec(boost::intmax_t const& ns)
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

    class timespec_duration
    {
    public:
      explicit timespec_duration(timespec const& v) : value(v) {}

#if defined BOOST_THREAD_USES_CHRONO
      timespec_duration(chrono::nanoseconds const& ns)
      {
        struct timespec d = { 0, 0 };
        d.tv_sec = static_cast<long>(chrono::duration_cast<chrono::seconds>(ns).count());
        d.tv_nsec = static_cast<long>((ns - chrono::duration_cast<chrono::seconds>(ns)).count());
        value =  d;
      }
#endif
      timespec& get() { return value; }
      timespec const& get() const { return value; }

      static inline timespec_duration zero()
      {
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        return timespec_duration(ts);
      }
    private:
      timespec value;
    };

    inline bool operator==(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) == to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator!=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) != to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator<(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) < to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator<=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) <= to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator>(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) > to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator>=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) >= to_nanoseconds_int_max(rhs.get());
    }

    class real_timespec_timepoint
    {
    public:
      explicit real_timespec_timepoint(timespec const& v) : value(v) {}

#if defined BOOST_THREAD_USES_DATETIME
      real_timespec_timepoint(boost::system_time const& abs_time)
      {
        boost::posix_time::time_duration const time_since_epoch = abs_time-boost::posix_time::from_time_t(0);

        struct timespec t = { 0, 0 };
        t.tv_sec = time_since_epoch.total_seconds();
        t.tv_nsec = (long)(time_since_epoch.fractional_seconds()*(1000000000l/time_since_epoch.ticks_per_second()));
        value = t;
      }
#endif
#if defined BOOST_THREAD_USES_CHRONO
      real_timespec_timepoint(chrono::time_point<chrono::system_clock, chrono::nanoseconds> const& abs_time)
      {
        using namespace chrono;
        nanoseconds ns = abs_time.time_since_epoch();
        struct timespec d = { 0, 0 };
        d.tv_sec = static_cast<long>(chrono::duration_cast<chrono::seconds>(ns).count());
        d.tv_nsec = static_cast<long>((ns - chrono::duration_cast<chrono::seconds>(ns)).count());
        value =  d;

      }
#endif

      timespec& get() { return value; }
      timespec const& get() const { return value; }
    private:
      timespec value;
    };

    inline bool operator==(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) == to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator!=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) != to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator<(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) < to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator<=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) <= to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator>(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) > to_nanoseconds_int_max(rhs.get());
    }
    inline bool operator>=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return to_nanoseconds_int_max(lhs.get()) >= to_nanoseconds_int_max(rhs.get());
    }

    inline real_timespec_timepoint operator+(real_timespec_timepoint const& lhs, timespec_duration const& rhs)
    {
      // fixme: replace by to_timespec_duration
      return real_timespec_timepoint(int_to_timespec(to_nanoseconds_int_max(lhs.get()) + to_nanoseconds_int_max(rhs.get())));
    }
    inline real_timespec_timepoint operator+(timespec_duration const& lhs, real_timespec_timepoint const& rhs)
    {
      // fixme: replace by to_timespec_duration
      return real_timespec_timepoint(int_to_timespec(to_nanoseconds_int_max(lhs.get()) + to_nanoseconds_int_max(rhs.get())));
    }
    inline timespec_duration operator-(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      // fixme: replace by to_timespec_duration
      return timespec_duration(int_to_timespec(to_nanoseconds_int_max(lhs.get()) - to_nanoseconds_int_max(rhs.get())));
    }

    struct real_timespec_clock
    {
      static inline real_timespec_timepoint now()
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
        return real_timespec_timepoint(ts);
      }
    };

#if defined BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
  class mono_timespec_timepoint
  {
  public:
    explicit mono_timespec_timepoint(timespec const& v) : value(v) {}

    timespec& get() { return value; }
    timespec const& get() const { return value; }
  private:
    timespec value;
  };

  inline bool operator==(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) == to_nanoseconds_int_max(rhs.get());
  }
  inline bool operator!=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) != to_nanoseconds_int_max(rhs.get());
  }
  inline bool operator<(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) < to_nanoseconds_int_max(rhs.get());
  }
  inline bool operator<=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) <= to_nanoseconds_int_max(rhs.get());
  }
  inline bool operator>(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) > to_nanoseconds_int_max(rhs.get());
  }
  inline bool operator>=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return to_nanoseconds_int_max(lhs.get()) >= to_nanoseconds_int_max(rhs.get());
  }

  inline mono_timespec_timepoint operator+(mono_timespec_timepoint const& lhs, timespec_duration const& rhs)
  {
    // fixme: replace by to_timespec_duration
    return mono_timespec_timepoint(int_to_timespec(to_nanoseconds_int_max(lhs.get()) + to_nanoseconds_int_max(rhs.get())));
  }
  inline mono_timespec_timepoint operator+(timespec_duration const& lhs, mono_timespec_timepoint const& rhs)
  {
    // fixme: replace by to_timespec_duration
    return mono_timespec_timepoint(int_to_timespec(to_nanoseconds_int_max(lhs.get()) + to_nanoseconds_int_max(rhs.get())));
  }
  inline timespec_duration operator-(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    // fixme: replace by to_timespec_duration
    return timespec_duration(int_to_timespec(to_nanoseconds_int_max(lhs) - to_nanoseconds_int_max(rhs)));
  }

  struct mono_timespec_clock
  {
    static inline mono_timespec_timepoint now()
    {
      timespec ts;
      if ( ::clock_gettime( CLOCK_MONOTONIC, &ts ) )
      {
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        BOOST_ASSERT(0 && "Boost::Thread - Internal Error");
      }
      return mono_timespec_timepoint(ts);
    }
  };
#endif

#if defined BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
  typedef mono_timespec_clock internal_timespec_clock;
  typedef mono_timespec_timepoint internal_timespec_timepoint;
#else
  typedef real_timespec_clock internal_timespec_clock;
  typedef real_timespec_timepoint internal_timespec_timepoint;
#endif

  }
}

#include <boost/config/abi_suffix.hpp>

#endif
