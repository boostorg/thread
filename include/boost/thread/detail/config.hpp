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

#endif // BOOST_THREAD_CONFIG_WEK1032003_HPP
