// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06041006_HPP
#define BOOST_THREAD_RS06041006_HPP

#include <boost/thread/win32/config.hpp>

#include <boost/cstdint.hpp>

namespace boost {

enum xtime_clock_types
{
    TIME_UTC=1
//    TIME_TAI,
//    TIME_MONOTONIC,
//    TIME_PROCESS,
//    TIME_THREAD,
//    TIME_LOCAL,
//    TIME_SYNC,
//    TIME_RESOLUTION
};

struct xtime
{
#if defined(BOOST_NO_INT64_T)
    typedef int_fast32_t xtime_sec_t; //INT_FAST32_MIN <= sec <= INT_FAST32_MAX
#else
    typedef int_fast64_t xtime_sec_t; //INT_FAST64_MIN <= sec <= INT_FAST64_MAX
#endif

    typedef int_fast32_t xtime_nsec_t; //0 <= xtime.nsec < NANOSECONDS_PER_SECOND

    xtime_sec_t sec;
    xtime_nsec_t nsec;
};

int BOOST_THREAD_DECL xtime_get(struct xtime* xtp, int clock_type);

inline int xtime_cmp(const xtime& xt1, const xtime& xt2)
{
    if (xt1.sec == xt2.sec)
        return (int)(xt1.nsec - xt2.nsec);
    else 
        return (xt1.sec > xt2.sec) ? 1 : -1;
}

} // namespace boost

#endif //BOOST_THREAD_RS06041006_HPP
