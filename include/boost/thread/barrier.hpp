// Copyright (C) 2002-2003
// David Moore, William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_BARRIER_JDM030602_HPP
#define BOOST_BARRIER_JDM030602_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace boost {

class BOOST_THREAD_DECL barrier
{
public:
    barrier(unsigned int count);
    ~barrier();

    bool wait();

private:
    mutex m_mutex;
    condition m_cond;
    unsigned int m_threshold;
    unsigned int m_count;
    unsigned int m_generation;
};

}   // namespace boost

#endif
