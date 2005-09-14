#ifndef BOOST_THREAD_DETAIL_XTIME_UTILS_HPP
#define BOOST_THREAD_DETAIL_XTIME_UTILS_HPP

//  xtime_utils.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/xtime.hpp>

namespace boost
{
    namespace detail
    {
        inline int get_milliseconds_until_time(boost::xtime target)
        {
            boost::xtime now;
            boost::xtime_get(&now, boost::TIME_UTC);

            if(boost::xtime_cmp(target,now)<=0)
            {
                return 0;
            }
            else
            {
                long const nanoseconds_per_second=1000000000;
                long const milliseconds_per_second=1000;
                long const nanoseconds_per_millisecond=nanoseconds_per_second/milliseconds_per_second;
                
                if(target.nsec<now.nsec)
                {
                    target.nsec+=nanoseconds_per_second;
                    --target.sec;
                }
                target.sec-=now.sec;
                target.nsec-=now.nsec;
                
                return static_cast<int>(target.sec*milliseconds_per_second)+
                    static_cast<int>((target.nsec+(nanoseconds_per_millisecond/2))/nanoseconds_per_millisecond);
            }
        }
    }
}


#endif
