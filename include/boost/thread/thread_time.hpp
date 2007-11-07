#ifndef BOOST_THREAD_TIME_HPP
#define BOOST_THREAD_TIME_HPP
//  (C) Copyright 2007 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/date_time/microsec_time_clock.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace boost
{
    typedef boost::posix_time::ptime system_time;
    
    inline system_time get_system_time()
    {
        return boost::date_time::microsec_clock<system_time>::universal_time();
    }

    namespace detail
    {
        inline system_time get_system_time_sentinel()
        {
            return system_time(boost::posix_time::pos_infin);
        }

        inline unsigned long get_milliseconds_until(system_time const& target_time)
        {
            if(target_time.is_pos_infinity())
            {
                return ~(unsigned long)0;
            }
            system_time const now=get_system_time();
            if(target_time<=now)
            {
                return 0;
            }
            return static_cast<unsigned long>((target_time-now).total_milliseconds()+1);
        }

    }
    
}

#endif
