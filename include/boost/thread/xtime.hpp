/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *   8 Feb 01  Initial version.
 */
 
#ifndef BOOST_XTIME_HPP
#define BOOST_XTIME_HPP

#include <boost/stdint.h>

namespace boost
{
    enum
    {
        TIME_UTC=1,
        TIME_TAI,
        TIME_MONOTONIC,
        TIME_PROCESS,
        TIME_THREAD,
        TIME_LOCAL,
        TIME_SYNC,
        TIME_RESOLUTION
    };

    struct xtime
    {
#if defined(BOOST_NO_INT64_T)
        int_fast32_t sec;
#else
        int_fast64_t sec;
#endif
        int_fast32_t nsec;
    };

    int xtime_get(struct xtime* xtp, int clock_type);
}

#endif // BOOST_XTIME_HPP