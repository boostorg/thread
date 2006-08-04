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
			// @Anthony: this might give an invalid xtime, since the numbers
			// might be negative, depending whether sec is an 64 or 32 bit int.
			// Also in case it is 64 bit, this will be smaller than other valid
			// time specifications. I know, the problem will be noticeable in
			// about 100 years from now, but ...
            boost::xtime sentinel={
                0xffffffff,0xffffffff
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
                return std::numeric_limits<unsigned long>::max();
            }
            
            boost::xtime now;
            boost::xtime_get(&now, boost::TIME_UTC);

			if (target.sec < now.sec)
			{
				return 0;
			}
			else
			{
				if (target.nsec < now.nsec) 
				{
					if (target.sec == now.sec)
					{
						return 0;
					}
					target.nsec = 1000000000 - now.nsec + target.nsec;
					target.sec = target.sec - now.sec - 1;
				}
				else
				{
					target.nsec -= now.nsec;
					target.sec -= now.sec;
				}
				// we are throwing away some bits, but one second after having 
				// waited for 49 years does not really matter ...
				if (target.sec < std::numeric_limits<unsigned long>::max()/1000)
				{
					// this cast is safe, since the result can be represented
					// as unsigned long
					return static_cast<unsigned long>(
						target.sec*1000 + (target.nsec+500000)/1000000	);
				}
				else
				{
					return std::numeric_limits<unsigned long>::max();
				}
			}
        }
    }
}


#endif
