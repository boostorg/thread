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
//
// (C) Copyright 2005 Anthony Williams

#ifndef BOOST_ONCE_WEK080101_HPP
#define BOOST_ONCE_WEK080101_HPP

#include <boost/thread/detail/config.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

#if defined(BOOST_HAS_WINTHREADS) 

#include <boost/thread/detail/once_win32.hpp>

#else

namespace boost {

#if defined(BOOST_HAS_PTHREADS)

typedef pthread_once_t once_flag;
#define BOOST_ONCE_INIT PTHREAD_ONCE_INIT

#endif

void BOOST_THREAD_DECL call_once(void (*func)(), once_flag& flag);

} // namespace boost

#endif

// Change Log:
//   1 Aug 01  WEKEMPF Initial version.
//   6 Sep 05  Anthony Williams. Split win32 stuff into detail/once_win32.hpp

#endif // BOOST_ONCE_WEK080101_HPP
