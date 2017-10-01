#ifndef BOOST_THREAD_DETAIL_TIMESPEC_HPP
#define BOOST_THREAD_DETAIL_TIMESPEC_HPP
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

#include <limits>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail
  {
#if defined BOOST_THREAD_PLATFORM_PTHREAD
    inline timespec ns_to_timespec(boost::intmax_t const& ns)
    {
      boost::intmax_t s = ns / 1000000000l;
      timespec ts;
      ts.tv_sec = static_cast<long> (s);
      ts.tv_nsec = static_cast<long> (ns - s * 1000000000l);
      return ts;
    }
    inline boost::intmax_t timespec_to_ns(timespec const& ts)
    {
      return static_cast<boost::intmax_t>(ts.tv_sec) * 1000000000l + ts.tv_nsec;
    }
#endif

    class timespec_duration
    {
    public:
#if defined BOOST_THREAD_PLATFORM_PTHREAD
      explicit timespec_duration(timespec const& v) : ts_val(v) {}
      inline timespec const& getTs() const { return ts_val; }

      explicit timespec_duration(boost::intmax_t const& ns = 0) : ts_val(ns_to_timespec(ns)) {}
      inline boost::intmax_t getNs() const { return timespec_to_ns(ts_val); }
#else
      explicit timespec_duration(boost::intmax_t const& ns = 0) : ns_val(ns) {}
      inline boost::intmax_t getNs() const { return ns_val; }
#endif

#if defined BOOST_THREAD_USES_DATETIME
      timespec_duration(boost::posix_time::time_duration const& rel_time)
      {
#if defined BOOST_THREAD_PLATFORM_PTHREAD
        ts_val.tv_sec = rel_time.total_seconds();
        ts_val.tv_nsec = static_cast<long>(rel_time.fractional_seconds() * (1000000000l / rel_time.ticks_per_second()));
#else
        ns_val = static_cast<boost::intmax_t>(rel_time.total_seconds()) * 1000000000l;
        ns_val += rel_time.fractional_seconds() * (1000000000l / rel_time.ticks_per_second());
#endif
      }
#endif

#if defined BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      timespec_duration(chrono::duration<Rep, Period> const& d)
      {
#if defined BOOST_THREAD_PLATFORM_PTHREAD
        ts_val = ns_to_timespec(chrono::ceil<chrono::nanoseconds>(d).count());
#else
        ns_val = chrono::ceil<chrono::nanoseconds>(d).count();
#endif
      }
#endif

      inline boost::intmax_t getMs() const
      {
        const boost::intmax_t ns = getNs();
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
        return timespec_duration(0);
      }

    private:
#if defined BOOST_THREAD_PLATFORM_PTHREAD
      timespec ts_val;
#else
      boost::intmax_t ns_val;
#endif
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
#if defined BOOST_THREAD_PLATFORM_PTHREAD
      explicit real_timespec_timepoint(timespec const& v) : dur(v) {}
      inline timespec const& getTs() const { return dur.getTs(); }
#endif

      explicit real_timespec_timepoint(boost::intmax_t const& ns) : dur(ns) {}
      inline boost::intmax_t getNs() const { return dur.getNs(); }

#if defined BOOST_THREAD_USES_DATETIME
      real_timespec_timepoint(boost::system_time const& abs_time)
        : dur(abs_time - boost::posix_time::from_time_t(0)) {}
#endif

#if defined BOOST_THREAD_USES_CHRONO
      template <class Duration>
      real_timespec_timepoint(chrono::time_point<chrono::system_clock, Duration> const& abs_time)
        : dur(abs_time.time_since_epoch()) {}
#endif

    private:
      timespec_duration dur;
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
#if defined BOOST_THREAD_PLATFORM_PTHREAD
    explicit mono_timespec_timepoint(timespec const& v) : dur(v) {}
    inline timespec const& getTs() const { return dur.getTs(); }
#endif

    explicit mono_timespec_timepoint(boost::intmax_t const& ns) : dur(ns) {}
    inline boost::intmax_t getNs() const { return dur.getNs(); }

#if defined BOOST_THREAD_USES_CHRONO
    // This conversion assumes that chrono::steady_clock::time_point and mono_timespec_timepoint share the same epoch.
    template <class Duration>
    mono_timespec_timepoint(chrono::time_point<chrono::steady_clock, Duration> const& abs_time)
      : dur(abs_time.time_since_epoch()) {}
#endif

    // can't name this max() since that is a macro on some Windows systems
    static inline mono_timespec_timepoint getMax()
    {
#if defined BOOST_THREAD_PLATFORM_PTHREAD
      timespec ts;
      ts.tv_sec = (std::numeric_limits<time_t>::max)();
      ts.tv_nsec = 999999999;
      return mono_timespec_timepoint(ts);
#else
      boost::intmax_t ns = (std::numeric_limits<boost::intmax_t>::max)();
      return mono_timespec_timepoint(ns);
#endif
    }

  private:
    timespec_duration dur;
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

#endif

#if defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
  typedef mono_timespec_clock internal_timespec_clock;
  typedef mono_timespec_timepoint internal_timespec_timepoint;
#else
  typedef real_timespec_clock internal_timespec_clock;
  typedef real_timespec_timepoint internal_timespec_timepoint;
#endif

#ifdef BOOST_THREAD_USES_CHRONO
#ifdef BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
  typedef chrono::steady_clock internal_chrono_clock;
#else
  typedef chrono::system_clock internal_chrono_clock;
#endif
#endif

  }
}

#include <boost/config/abi_suffix.hpp>

#endif
