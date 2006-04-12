// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2006 Anthony Williams
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/win32/config.hpp>

#if defined(BOOST_HAS_FTIME)
#   define __STDC_CONSTANT_MACROS
#endif

#include <boost/thread/xtime.hpp>

#if defined(BOOST_HAS_FTIME)
#   include <windows.h>
#   include <boost/cstdint.hpp>
#endif

namespace boost {


int xtime_get(struct xtime* xtp, int clock_type)
{
    if (clock_type == TIME_UTC)
    {
#if defined(BOOST_HAS_FTIME)
        FILETIME ft;
#   if defined(BOOST_NO_GETSYSTEMTIMEASFILETIME)
        {
            SYSTEMTIME st;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st,&ft);
        }
#   else
        GetSystemTimeAsFileTime(&ft);
#   endif
        static const boost::uint64_t TIMESPEC_TO_FILETIME_OFFSET =
            UINT64_C(116444736000000000);
        
        const boost::uint64_t ft64 =
            (static_cast<boost::uint64_t>(ft.dwHighDateTime) << 32)
            + ft.dwLowDateTime;

        xtp->sec = static_cast<xtime::xtime_sec_t>(
            (ft64 - TIMESPEC_TO_FILETIME_OFFSET) / 10000000
        );

        xtp->nsec = static_cast<xtime::xtime_nsec_t>(
            ((ft64 - TIMESPEC_TO_FILETIME_OFFSET) % 10000000) * 100
        );

        return clock_type;
#endif
    }
    return 0;
}

} // namespace boost
