// Copyright (C)  2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

// Derived loosely from work queue manager in "Programming POSIX Threads"
//   by David Butenhof.

#ifndef BOOST_THREAD_POOL_JDM031802_HPP
#define BOOST_THREAD_POOL_JDM031802_HPP

#include <boost/config.hpp>

// insist on threading support being available:
#include <boost/config/requires_threads.hpp>

#include <boost/thread/detail/config.hpp>

#include <boost/function.hpp>
#include <boost/limits.hpp>

namespace boost {

class BOOST_THREAD_DECL thread_pool
{
public:
    thread_pool(int max_threads=std::numeric_limits<int>::max(),
        int min_threads=0, int timeout_secs=5, int timeout_nsecs=0);
    ~thread_pool();

    void add(const boost::function0<void> &job);
    void join();
    void cancel();
    void detach();

private:
    class impl;
    impl* m_pimpl;
};

}   // namespace boost

#endif
