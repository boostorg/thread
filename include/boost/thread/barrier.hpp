// Copyright (C)  2002
// David Moore
//
// Original mutex and cv design and implementation for Boost.Threads
// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  David Moore makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//
//
// A Boost.Threads implementation of a synchronization 
//   primitive which allows a fixed number of threads to synchronize
//   their execution paths at selected "barriers"


#ifndef BOOST_RW_MUTEX_JDM030602_HPP
#define BOOST_RW_MUTEX_JDM030602_HPP

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>


namespace boost { 

    class barrier;

    namespace detail { namespace thread {
    class sub_barrier
    {
    public:
        sub_barrier(unsigned int count);
        ~sub_barrier();
    private:
        unsigned int m_running;
        boost::condition m_barrier_finished;

        friend class boost::barrier;
    };
    } // namespace thread
    } // namespace detail

const int BOOST_SERIAL_THREAD = -1;

class barrier
{
public:
    barrier(unsigned int count);
    int wait();
    ~barrier();     
private:

    unsigned int m_generation;
    unsigned int m_count;
    detail::thread::sub_barrier m_sub_1;
    detail::thread::sub_barrier m_sub_2;
    detail::thread::sub_barrier *m_subs[2];

    mutex m_mutex;
};




class one_shot_barrier 
{
public:

    one_shot_barrier(unsigned int count);
    ~one_shot_barrier();

    int wait();
private:
    boost::condition m_barrier_finished;
    boost::mutex m_mutex;
    unsigned int m_running;     // How many threads running?
};

}   // namespace boost


#endif
