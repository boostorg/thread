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

#include <boost/thread/xtime.hpp>

#if defined(BOOST_HAS_FTIME)
#   include <windows.h>
#elif defined(BOOST_HAS_GETTIMEOFDAY)
#	include <sys/time.h>
#endif

namespace boost {

int xtime_get(struct xtime* xtp, int clock_type)
{
    if (clock_type == TIME_UTC)
    {
#if defined(BOOST_HAS_FTIME)
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        const __int64 TIMESPEC_TO_FILETIME_OFFSET = ((__int64)27111902 << 32) + (__int64)3577643008;
        xtp->sec = (int)((*(__int64*)&ft - TIMESPEC_TO_FILETIME_OFFSET) / 10000000);
        xtp->nsec = (int)((*(__int64*)&ft - TIMESPEC_TO_FILETIME_OFFSET -
            ((__int64)xtp->sec * (__int64)10000000)) * 100);
        return clock_type;
#elif defined(BOOST_HAS_GETTIMEOFDAY)
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
#   error "xtime_get implementation undefined"
#endif
    }
    return 0;
}

} // namespace boost

// Change Log:
//   8 Feb 01  WEKEMPF Initial version.
