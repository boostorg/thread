#ifndef BOOST_READ_WRITE_SCHEDULING_POLICY_HPP
#define BOOST_READ_WRITE_SCHEDULING_POLICY_HPP

//  read_write_scheduling_policy.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

namespace boost
{
    namespace read_write_scheduling_policy
    {
        enum read_write_scheduling_policy_enum
        {
            writer_priority,
            reader_priority,
            alternating_many_reads,
            alternating_single_read
        };
    }
}

#endif
