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


#include <boost/thread/barrier.hpp>


namespace boost {



barrier::
barrier(unsigned int count) : 
    m_count(count), m_generation(0), 
    m_sub_1(count), m_sub_2(count)
{
    if(count == 0)
        throw std::invalid_argument("count cannot be zero.");

    m_subs[0] = &m_sub_1;
    m_subs[1] = &m_sub_2;
}

barrier::
~barrier()
{   
    boost::mutex::scoped_lock l(m_mutex);
    // This is our "signal" that we've been destroyed.
    m_subs[0] = m_subs[1] = NULL;
}


int
barrier::
wait()
{
    int ret;
    boost::mutex::scoped_lock l(m_mutex);

    detail::thread::sub_barrier *psub = m_subs[m_generation];
    if(psub == NULL)
        throw lock_error();

    if(psub->m_running == 1)
    {
        // This is the last running thread for the current
        // generation.

        // reset that generation's count to our full count for next time
        psub->m_running = m_count;
        m_generation = 1-m_generation;
        psub->m_barrier_finished.notify_all();
        ret = BOOST_SERIAL_THREAD;        // We're the one who woke them up
    }
    else 
    {
        // decrement the count for that generation
        --psub->m_running;
        // wait to see the condition where we are "reset" by the 
        //    last waiter...
        while(psub->m_running != m_count)
            psub->m_barrier_finished.wait(l);
        ret = 0;        // We were notified to wake up
    }

    return ret;
}


namespace detail { namespace thread {
sub_barrier::
sub_barrier(unsigned int count) :
    m_running(count)
{

}	

sub_barrier::
~sub_barrier()
{
}

} // namespace thread
} // namespace detail



one_shot_barrier::
one_shot_barrier(unsigned int count) : m_running(count)
{
}

one_shot_barrier::
~one_shot_barrier()
{
}


int 
one_shot_barrier::
wait()
{
    int ret;
    boost::mutex::scoped_lock l(m_mutex);

    if(m_running == 1)
    {
        // we are the last waiting thread.
        ret = BOOST_SERIAL_THREAD;
        m_running = 0;
        m_barrier_finished.notify_all();
    }
    else if(m_running != 0)
    {
        --m_running;

        while(m_running != 0)
            m_barrier_finished.wait(l);
        ret = 0;
    }
    else
    {
        // wait has been called "too many" times.
        //
        // For this simple barrier, we don't support generations
        //   of waiting, so there are two choices here - ignore and allow
        //   the caller to proceed, or throw an exception..
        //
        throw lock_error();
    }
    return ret;
}

    
}// namespace boost


