#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#ifdef BOOST_HAS_WINTHREADS

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the BOOST_THREADMON_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// BOOST_THREADMON_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef BOOST_THREADMON_EXPORTS
#define BOOST_THREADMON_API __declspec(dllexport)
#else
#define BOOST_THREADMON_API __declspec(dllimport)
#endif

extern "C" BOOST_THREADMON_API int on_thread_exit(void (__cdecl * func)(void));

#endif // BOOST_HAS_WINTHREADS
