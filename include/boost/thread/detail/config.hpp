// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_THREAD_CONFIG_WEK01032003_HPP
#define BOOST_THREAD_CONFIG_WEK01032003_HPP

#include <boost/config.hpp>

// insist on threading support being available:
#include <boost/config/requires_threads.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#   if defined(BOOST_THREAD_BUILD_DLL) //Build dll
#       define BOOST_THREAD_DECL __declspec(dllexport)
#   elif defined(BOOST_THREAD_BUILD_LIB) //Build lib
#       define BOOST_THREAD_DECL
#       define BOOST_THREAD_NO_TSS_CLEANUP
#   elif defined(BOOST_THREAD_USE_LIB) //Use lib
#       define BOOST_THREAD_DECL
#       define BOOST_THREAD_NO_TSS_CLEANUP
#   else //Use dll
#       define BOOST_THREAD_DECL __declspec(dllimport)
#       define BOOST_DYN_LINK
#   endif
#else
#   define BOOST_THREAD_DECL
#   if defined(BOOST_THREAD_USE_LIB) //Use lib
#   else //Use dll
#       define BOOST_DYN_LINK
#   endif
#endif // BOOST_HAS_WINTHREADS

//
// Automatically link to the correct build variant where possible. 
// 
#if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_THREAD_NO_LIB) && !defined(BOOST_THREAD_BUILD_DLL) && !defined(BOOST_THREAD_BUILD_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#if defined(BOOST_THREAD_LIB_NAME)
#    define BOOST_LIB_NAME BOOST_THREAD_LIB_NAME
#else
#    define BOOST_LIB_NAME boost_thread
#endif
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#endif // BOOST_THREAD_CONFIG_WEK1032003_HPP
