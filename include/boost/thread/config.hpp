// This file is used to configure Boost.Threads during development
// in order to decouple dependency on any Boost release.  Once
// accepted into Boost these contents will be moved to <boost/config>
// or some other appropriate build configuration and all
// #include <boost/thread/config.hpp> statements will be changed
// accordingly.

#ifndef BOOST_THREAD_CONFIG_HPP
#define BOOST_THREAD_CONFIG_HPP

#include <boost/config.hpp>

// Define if threading support is enabled for the toolset.
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
//#   define BOOST_HAS_WINTHREADS // comment out this to test pthreads-win32.
#   if !defined(BOOST_HAS_WINTHREADS)
#       define BOOST_HAS_PTHREADS
#       define BOOST_HAS_PTHREAD_MUTEXATTR_SETTYPE
#       define PtW32NoCatchWarn
#       pragma comment(lib, "pthreadVCE.lib")
#   endif
#   define BOOST_HAS_FTIME
#endif

#endif