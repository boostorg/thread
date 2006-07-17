// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>
#include <boost/thread/pthread/xtime.hpp>

// TODO: xtime possible should be replaced by boost date time
#if defined(BOOST_HAS_GETTIMEOFDAY)
#   include <sys/time.h>
#elif ! defined (BOOST_HAS_CLOCK_GETTIME)
#   include <time.h>
#endif
 
namespace boost {

int xtime_get(struct xtime* xtp, int clock_type)
{
    if (clock_type == TIME_UTC)
    {
#if defined(BOOST_HAS_GETTIMEOFDAY)
        struct timeval tv;
        gettimeofday(&tv, 0);
        xtp->sec = tv.tv_sec;
        xtp->nsec = tv.tv_usec * 1000;
        return clock_type;
#elif defined(BOOST_HAS_CLOCK_GETTIME)
        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        xtp->sec = ts.tv_sec;
        xtp->nsec = ts.tv_nsec;
        return clock_type;
#else
	time_t t;
	time(&t);
	xtp->sec  = t;
	xtp->nsec = 0;
	return clock_type;
#endif
    }
    return 0;
}

} // namespace boost
