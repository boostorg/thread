#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#include <boost/thread/detail/config.hpp>

#ifdef BOOST_HAS_WINTHREADS

#include <boost/thread/detail/config.hpp>

extern "C" BOOST_THREAD_DECL int on_thread_exit(void (__cdecl * func)(void));

#endif // BOOST_HAS_WINTHREADS
