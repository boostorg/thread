// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

// This file is used to configure Boost.Threads during development
// in order to decouple dependency on any Boost release.  Once
// accepted into Boost these contents will be moved to <boost/config>
// or some other appropriate build configuration and all
// #include <boost/thread/config.hpp> statements will be changed
// accordingly.

#ifndef BOOST_THREAD_CONFIG_WEK070601_HPP
#define BOOST_THREAD_CONFIG_WEK070601_HPP

#include <boost/config.hpp>

#error "Included <boost/thread/config.hpp>"

/*// Define if threading support is enabled for the toolset.
#undef BOOST_HAS_THREADS

// Define if threading should be implemented in terms of Win32 threads.
#undef BOOST_HAS_WINTHREADS

// Define if threading should be implemented in terms of POSIX threads.
#undef BOOST_HAS_PTHREADS

// Define if BOOST_HAS_PTHREADS and pthread_delay_np() exists.
#undef BOOST_HAS_PTHREAD_DELAY_NP

// Define if BOOST_HAS_PTHREADS and not BOOST_HAS_PTHREAD_DELAY_NP
// but nanosleep can be used instead.
#undef BOOST_HAS_NANOSLEEP

// Define if BOOST_HAS_PTHREADS and pthread_yield() exists.
#undef BOOST_HAS_PTHREAD_YIELD

// Define if BOOST_HAS_PTHREADS and not BOOST_HAS_PTHREAD_YIELD and
// sched_yield() exists.
#undef BOOST_HAS_SCHED_YIELD

// Define if gettimeofday() exists.
#undef BOOST_HAS_GETTIMEOFDAY

// Define if not BOOST_HAS_GETTIMEOFDAY and clock_gettime() exists.
#undef BOOST_HAS_CLOCK_GETTIME

// Define if not BOOST_HAS_GETTIMEOFDAY and not BOOST_HAS_CLOCK_GETTIME and
// GetSystemTimeAsFileTime() can be called with an FTIME structure.
#undef BOOST_HAS_FTIME

// Define if pthread_mutexattr_settype and pthread_mutexattr_gettype exist.
#undef BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE

// Here we'll set up known compiler options.

#if defined(BOOST_MSVC)
#   if defined(_MT)
#       define BOOST_HAS_THREADS
#   endif
#   define BOOST_HAS_WINTHREADS // comment out this to test pthreads-win32.
#   if !defined(BOOST_HAS_WINTHREADS)
#       define BOOST_HAS_PTHREADS
#       define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#       define PtW32NoCatchWarn
#       pragma comment(lib, "pthreadVCE.lib")
#   endif
#   define BOOST_HAS_FTIME
    //  pdm: this is for linux - is there a better #define to #if on?
    //  wek: not sure how else to do this, but GNU CC on Win32 should probably
    //       use BOOST_HAS_WINTHREADS, and I expect there will be other
    //       platform specific variations for this compiler toolset.  Need
    //       to decide how to handle this.
#elif defined( __GNUC__ )
#   define BOOST_HAS_THREADS
#   define BOOST_HAS_PTHREADS
#   define BOOST_HAS_NANOSLEEP
#   define BOOST_HAS_GETTIMEOFDAY
    //  pdm: From the pthread.h header, one of these macros
    //  must be defined for this stuff to exist.
    //  wek: This seems like a harmless enough method to determine these
    //       switches, but one should note that some implementations may not
    //       use these.  Notably, pthreads-win32 doesn't define either
    //       __USE_UNIX98 or __USE_GNU.
#   if defined( __USE_UNIX98 )
#       define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#   elif defined( __USE_GNU )
#       define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#       define BOOST_HAS_PTHREAD_YIELD
#   endif
#endif*/

#endif // BOOST_THREAD_CONFIG_WEK070601_HPP
