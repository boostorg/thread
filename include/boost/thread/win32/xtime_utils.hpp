#ifndef BOOST_THREAD_DETAIL_XTIME_UTILS_HPP
#define BOOST_THREAD_DETAIL_XTIME_UTILS_HPP

//  xtime_utils.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/win32/xtime.hpp>
#include <limits>

namespace boost
{
    namespace detail
    {
        inline ::boost::xtime get_xtime_sentinel()
        {
            boost::xtime const sentinel={
                (std::numeric_limits<boost::xtime::xtime_sec_t>::max)(),
                (std::numeric_limits<boost::xtime::xtime_nsec_t>::max)()
            };
            return sentinel;
            
        }
        
        // get the number of milliseconds from now to target
        // if target is in the past, return 0
        // if target cannot be represented as unsigned long, return
        // the maximum instead
        // 2006-08-04 <roland>
        inline unsigned long get_milliseconds_until_time(::boost::xtime target)
        {
            if(!boost::xtime_cmp(target,get_xtime_sentinel()))
            {
                return (std::numeric_limits<unsigned long>::max)();
            }
            
            boost::xtime now;
            boost::xtime_get(&now, boost::TIME_UTC);

            if (target.sec < now.sec)
            {
                return 0;
            }
            else
            {
                boost::xtime::xtime_nsec_t const nanoseconds_per_second=1000000000;
                boost::xtime::xtime_nsec_t const milliseconds_per_second=1000;
                boost::xtime::xtime_nsec_t const nanoseconds_per_millisecond=nanoseconds_per_second/milliseconds_per_second;
                if (target.nsec < now.nsec) 
                {
                    if (target.sec == now.sec)
                    {
                        return 0;
                    }
                    target.nsec += nanoseconds_per_second - now.nsec;
                    target.sec -= now.sec + 1;
                }
                else
                {
                    target.nsec -= now.nsec;
                    target.sec -= now.sec;
                }
                // we are throwing away some bits, but one second after having 
                // waited for 49 years does not really matter ...
                if (target.sec < (std::numeric_limits<unsigned long>::max)()/milliseconds_per_second)
                {
                    return static_cast<unsigned long>(
                        target.sec*milliseconds_per_second + 
                        (target.nsec+nanoseconds_per_millisecond/2)/nanoseconds_per_millisecond);
                }
                else
                {
                    return (std::numeric_limits<unsigned long>::max)();
                }
            }
        }
    }
}


#endif
