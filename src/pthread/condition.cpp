// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>
#include <boost/thread/pthread/condition.hpp>

#include <boost/thread/pthread/xtime.hpp>
#include <boost/thread/pthread/thread.hpp>
#include <boost/thread/exceptions.hpp>

#include <boost/limits.hpp>
#include <cassert>
#include <limits>

#include <errno.h>

namespace boost {

namespace detail {

condition_impl::condition_impl()
{
    int res = 0;
    res = pthread_cond_init(&m_condition, 0);
    if (res != 0)
        throw thread_resource_error();
}

condition_impl::~condition_impl()
{
    int res = 0;
    res = pthread_cond_destroy(&m_condition);
    assert(res == 0);
}

void condition_impl::notify_one()
{
    int res = 0;
    res = pthread_cond_signal(&m_condition);
    assert(res == 0);
}

void condition_impl::notify_all()
{
    int res = 0;
    res = pthread_cond_broadcast(&m_condition);
    assert(res == 0);
}

void condition_impl::do_wait(pthread_mutex_t* pmutex)
{
    int res = 0;
    res = pthread_cond_wait(&m_condition, pmutex);
    assert(res == 0);
}

bool condition_impl::do_timed_wait(const xtime& xt, pthread_mutex_t* pmutex)
{
    boost::xtime t;
    // normalize, in case user has specified nsec only
    if (xt.nsec > 999999999)
    {
        t.sec  = xt.nsec / 1000000000;
	t.nsec = xt.nsec % 1000000000;
    }
    else
    {
        t.sec = t.nsec = 0;
    }

    timespec ts;
    if (xt.sec < std::numeric_limits<time_t>::max() - t.sec) // avoid overflow
    {
        ts.tv_sec = static_cast<time_t>(xt.sec + t.sec);
	ts.tv_nsec = static_cast<long>(t.nsec);
    }
    else
    {   // saturate to avoid overflow
        ts.tv_sec = std::numeric_limits<time_t>::max();
	ts.tv_nsec = 999999999; // this should not overflow, or tv_nsec is odd anyways...
    }
//    to_timespec(xt, ts);

    int res = 0;
    res = pthread_cond_timedwait(&m_condition, pmutex, &ts);
    assert(res == 0 || res == ETIMEDOUT);

    return res != ETIMEDOUT;
}

} // namespace detail

} // namespace boost
