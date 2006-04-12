#ifndef BOOST_THREAD_BARRIER_HPP
#define BOOST_THREAD_BARRIER_HPP

//  barrier.hpp
//
//  (C) Copyright 2006 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <stdexcept>
#include <boost/thread/detail/platform.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
    using ::size_t;
}
#endif

namespace boost
{
    class barrier
    {
        boost::mutex m;
        boost::condition cond;
        
        const std::size_t max_count;
        std::size_t current_count;
    public:
        barrier(std::size_t count):
            max_count(count),current_count(0)
        {
            if(!max_count)
            {
                throw std::invalid_argument("You must specify a non-zero count");
            }
        }
        
        bool wait()
        {
            boost::mutex::scoped_lock lock(m);
            if(++current_count==max_count)
            {
                current_count=0;
                cond.notify_all();
                return true;
            }
            else
            {
                cond.wait(lock);
                return false;
            }
        }
        
    };
}


#endif
