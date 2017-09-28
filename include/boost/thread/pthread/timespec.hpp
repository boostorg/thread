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
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#if defined(BOOST_THREAD_PLATFORM_WIN32)
#include <boost/detail/winapi/time.hpp>
#include <boost/detail/winapi/timers.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#elif defined(BOOST_THREAD_MACOS)
#include <sys/time.h> //for gettimeofday and timeval
#else
#include <time.h>  // for clock_gettime
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail
  {

    inline timespec ns_to_timespec(boost::intmax_t const& ns)
    {
      boost::intmax_t s = ns / 1000000000l;
      struct timespec ts;
      ts.tv_sec = static_cast<long> (s);
      ts.tv_nsec = static_cast<long> (ns - s * 1000000000l);
      return ts;
    }
    inline boost::intmax_t timespec_to_ns(timespec const& ts)
    {
      return static_cast<boost::intmax_t>(ts.tv_sec) * 1000000000l + ts.tv_nsec;
    }

    class timespec_duration
    {
    public:
      explicit timespec_duration(timespec const& v) : value(v) {}
      explicit timespec_duration(boost::intmax_t const& ns) : value(ns_to_timespec(ns)) {}

#if defined BOOST_THREAD_USES_DATETIME
      timespec_duration(boost::posix_time::time_duration const& rel_time)
      {
        struct timespec d = { 0,0};
        d.tv_sec=rel_time.total_seconds();
        d.tv_nsec=(long)(rel_time.fractional_seconds()*(1000000000l/rel_time.ticks_per_second()));
        value =  d;
      }
#endif
#if defined BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      timespec_duration(chrono::duration<Rep, Period> const& d)
      {
        value.tv_sec  = static_cast<long>(chrono::duration_cast<chrono::seconds>(d).count());
        value.tv_nsec = static_cast<long>((chrono::ceil<chrono::nanoseconds>(d) - chrono::duration_cast<chrono::seconds>(d)).count());
      }
#endif

      timespec& get() { return value; }
      timespec const& get() const { return value; }
      boost::intmax_t getNs() const { return timespec_to_ns(value); }
      inline boost::intmax_t getMs() const
      {
        const boost::intmax_t& ns = timespec_to_ns(value);
        // ceil/floor away from zero
        if (ns >= 0)
        {
          // return ceiling of positive numbers
          return (ns + 999999) / 1000000;
        }
        else
        {
          // return floor of negative numbers
          return (ns - 999999) / 1000000;
        }
      }

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
      return lhs.getNs() == rhs.getNs();
    }
    inline bool operator!=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return lhs.getNs() != rhs.getNs();
    }
    inline bool operator<(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return lhs.getNs() < rhs.getNs();
    }
    inline bool operator<=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return lhs.getNs() <= rhs.getNs();
    }
    inline bool operator>(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return lhs.getNs() > rhs.getNs();
    }
    inline bool operator>=(timespec_duration const& lhs, timespec_duration const& rhs)
    {
      return lhs.getNs() >= rhs.getNs();
    }

    static inline timespec_duration timespec_milliseconds(long const& ms)
    {
      return timespec_duration(ms * 1000000l);
    }

    class real_timespec_timepoint
    {
    public:
      explicit real_timespec_timepoint(timespec const& v) : value(v) {}
      explicit real_timespec_timepoint(boost::intmax_t const& ns) : value(ns_to_timespec(ns)) {}

#if defined BOOST_THREAD_USES_DATETIME
      real_timespec_timepoint(boost::system_time const& abs_time)
      {
        boost::posix_time::time_duration const time_since_epoch = abs_time-boost::posix_time::from_time_t(0);
        value = timespec_duration(time_since_epoch).get();
      }
#endif
#if defined BOOST_THREAD_USES_CHRONO
      template <class Duration>
      real_timespec_timepoint(chrono::time_point<chrono::system_clock, Duration> const& abs_time)
      {
        value = timespec_duration(abs_time.time_since_epoch()).get();
      }
#endif

      inline timespec& get() { return value; }
      inline timespec const& get() const { return value; }
      inline boost::intmax_t getNs() const { return timespec_to_ns(value); }

    private:
      timespec value;
    };

    inline bool operator==(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() == rhs.getNs();
    }
    inline bool operator!=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() != rhs.getNs();
    }
    inline bool operator<(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() < rhs.getNs();
    }
    inline bool operator<=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() <= rhs.getNs();
    }
    inline bool operator>(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() > rhs.getNs();
    }
    inline bool operator>=(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return lhs.getNs() >= rhs.getNs();
    }

    inline real_timespec_timepoint operator+(real_timespec_timepoint const& lhs, timespec_duration const& rhs)
    {
      return real_timespec_timepoint(lhs.getNs() + rhs.getNs());
    }
    inline real_timespec_timepoint operator+(timespec_duration const& lhs, real_timespec_timepoint const& rhs)
    {
      return real_timespec_timepoint(lhs.getNs() + rhs.getNs());
    }
    inline timespec_duration operator-(real_timespec_timepoint const& lhs, real_timespec_timepoint const& rhs)
    {
      return timespec_duration(lhs.getNs() - rhs.getNs());
    }

    struct real_timespec_clock
    {
      static inline real_timespec_timepoint now()
      {
#if defined(BOOST_THREAD_PLATFORM_WIN32)
        boost::detail::winapi::FILETIME_ ft;
        boost::detail::winapi::GetSystemTimeAsFileTime(&ft);  // never fails
        boost::intmax_t ns = ((((static_cast<boost::intmax_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) - 116444736000000000LL) * 100LL);
        return real_timespec_timepoint(ns);
#elif defined(BOOST_THREAD_MACOS)
        timeval tv;
        ::gettimeofday(&tv, 0);
        timespec ts;
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
        return real_timespec_timepoint(ts);
#else
        timespec ts;
        if ( ::clock_gettime( CLOCK_REALTIME, &ts ) )
        {
          BOOST_ASSERT(0 && "Boost::Thread - clock_gettime(CLOCK_REALTIME) Internal Error");
          return real_timespec_timepoint(0);
        }
        return real_timespec_timepoint(ts);
#endif
      }
    };

#if defined(BOOST_THREAD_HAS_MONO_CLOCK)

  class mono_timespec_timepoint
  {
  public:
    explicit mono_timespec_timepoint(timespec const& v) : value(v) {}
    explicit mono_timespec_timepoint(boost::intmax_t const& ns) : value(ns_to_timespec(ns)) {}

#if defined BOOST_THREAD_USES_CHRONO
    template <class Duration>
    mono_timespec_timepoint(chrono::time_point<chrono::steady_clock, Duration> const& abs_time);
#endif

    inline timespec& get() { return value; }
    inline timespec const& get() const { return value; }
    inline boost::intmax_t getNs() const { return timespec_to_ns(value); }

    // can't use max() since that is a macro on some Windows systems
    static inline mono_timespec_timepoint getMax()
    {
      timespec ts;
      ts.tv_sec  = 0x7fffffff;
      ts.tv_nsec = 0x7fffffff;
      return mono_timespec_timepoint(ts);
    }

  private:
    timespec value;
  };

  inline bool operator==(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() == rhs.getNs();
  }
  inline bool operator!=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() != rhs.getNs();
  }
  inline bool operator<(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() < rhs.getNs();
  }
  inline bool operator<=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() <= rhs.getNs();
  }
  inline bool operator>(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() > rhs.getNs();
  }
  inline bool operator>=(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return lhs.getNs() >= rhs.getNs();
  }

  inline mono_timespec_timepoint operator+(mono_timespec_timepoint const& lhs, timespec_duration const& rhs)
  {
    return mono_timespec_timepoint(lhs.getNs() + rhs.getNs());
  }
  inline mono_timespec_timepoint operator+(timespec_duration const& lhs, mono_timespec_timepoint const& rhs)
  {
    return mono_timespec_timepoint(lhs.getNs() + rhs.getNs());
  }
  inline timespec_duration operator-(mono_timespec_timepoint const& lhs, mono_timespec_timepoint const& rhs)
  {
    return timespec_duration(lhs.getNs() - rhs.getNs());
  }

  struct mono_timespec_clock
  {
    static inline mono_timespec_timepoint now()
    {
#if defined(BOOST_THREAD_PLATFORM_WIN32)
#if defined(BOOST_THREAD_USES_CHRONO)
      // Use QueryPerformanceCounter() to match the implementation in Boost
      // Chrono so that chrono::steady_clock::now() and this function share the
      // same epoch and so can be converted between each other.
      boost::detail::winapi::LARGE_INTEGER_ freq;
      if ( !boost::detail::winapi::QueryPerformanceFrequency( &freq ) )
      {
        BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceFrequency Internal Error");
        return mono_timespec_timepoint(0);
      }
      if ( freq.QuadPart <= 0 )
      {
        BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceFrequency Internal Error");
        return mono_timespec_timepoint(0);
      }

      boost::detail::winapi::LARGE_INTEGER_ pcount;
      unsigned times=0;
      while ( ! boost::detail::winapi::QueryPerformanceCounter( &pcount ) )
      {
        if ( ++times > 3 )
        {
          BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceCounter Internal Error");
          return mono_timespec_timepoint(0);
        }
      }

      long double ns = 1000000000.0L * pcount.QuadPart / freq.QuadPart;
      return mono_timespec_timepoint(static_cast<boost::intmax_t>(ns));
#else
      // Use GetTickCount64() because it's more reliable on older
      // systems like Windows XP and Windows Server 2003.
      win32::ticks_type msec = win32::GetTickCount64_()();
      return mono_timespec_timepoint(msec * 1000000);
#endif
#elif defined(BOOST_THREAD_MACOS)
      // fixme: add support for mono_timespec_clock::now() on MAC OS X using code from
      // https://github.com/boostorg/chrono/blob/develop/include/boost/chrono/detail/inlined/mac/chrono.hpp
      // Also update BOOST_THREAD_HAS_MONO_CLOCK in config.hpp
      return mono_timespec_timepoint(0);
#else
      timespec ts;
      if ( ::clock_gettime( CLOCK_MONOTONIC, &ts ) )
      {
        BOOST_ASSERT(0 && "Boost::Thread - clock_gettime(CLOCK_MONOTONIC) Internal Error");
        return mono_timespec_timepoint(0);
      }
      return mono_timespec_timepoint(ts);
#endif
    }
  };

#if defined BOOST_THREAD_USES_CHRONO
  template <class Duration>
  mono_timespec_timepoint::mono_timespec_timepoint(chrono::time_point<chrono::steady_clock, Duration> const& abs_time)
  {
    // This conversion assumes that chrono::steady_clock::now() and
    // mono_timespec_clock::now() share the same epoch.
    value = timespec_duration(abs_time.time_since_epoch()).get();
  }
#endif

#endif

#if defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
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
