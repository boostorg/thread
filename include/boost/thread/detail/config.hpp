#ifndef BOOST_THREAD_CONFIG_WEK01032003_HPP
#define BOOST_THREAD_CONFIG_WEK01032003_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#if defined(BOOST_HAS_WINTHREADS)
#   if defined(BOOST_THREAD_BUILD_DLL)
#       define BOOST_THREAD_DECL __declspec(dllexport)
#   else
#       define BOOST_THREAD_DECL __declspec(dllimport)
#   endif
#else
#   define BOOST_THREAD_DECL
#endif // BOOST_THREAD_SHARED_LIB

#if defined(BOOST_HAS_WINTHREADS)
#   define BOOST_THREAD_ATTRIBUTES_STACKSIZE
#   define BOOST_THREAD_STACK_MIN 0
#   define BOOST_THREAD_PRIORITY_SCHEDULING
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_ATTR_STACKSIZE)
#       define BOOST_THREAD_ATTRIBUTES_STACKSIZE
#       define BOOST_THREAD_STACK_MIN PTHREAD_STACK_MIN
#   endif
#   if defined(_POSIX_THREAD_ATTR_STACKADDR)
#       define BOOST_THREAD_ATTRIBUTES_STACKADDR
#   endif
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
#       define BOOST_THREAD_PRIORITY_SCHEDULING
#   endif
#endif

#endif // BOOST_THREAD_CONFIG_WEK1032003_HPP
