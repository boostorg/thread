// Copyright (C)  2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  David Moore makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

// A Boost::threads implementation of a synchronization
//   primitive which can allow multiple readers or a single
//   writer to have access to a shared resource.

#ifndef BOOST_RW_MUTEX_JDM030602_HPP
#define BOOST_RW_MUTEX_JDM030602_HPP

#include <boost/config.hpp>

// insist on threading support being available:
#include <boost/config/requires_threads.hpp>

#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/detail/lock.hpp>
#include <boost/thread/detail/rw_lock.hpp>
#include <boost/thread/condition.hpp>

namespace boost {

typedef enum
{
    sp_writer_priority,
    sp_reader_priority,
    sp_alternating_many_reads,
    sp_alternating_single_reads
} rw_scheduling_policy;

namespace detail {

namespace thread {

// Shared implementation construct for explicit Scheduling Policies
// This implementation is susceptible to self-deadlock, though....
template<typename Mutex>
struct rw_mutex_impl
{
    typedef detail::thread::scoped_lock<Mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<Mutex> scoped_try_lock;
    typedef detail::thread::scoped_timed_lock<Mutex> scoped_timed_lock;

    rw_mutex_impl(rw_scheduling_policy sp)
        : m_num_waiting_writers(0),
          m_num_waiting_readers(0),
          m_num_waiting_promotion(0),
          m_state(0),
          m_sp(sp),
          m_readers_next(1) { }

    Mutex m_prot;
    boost::condition m_waiting_writers;
    boost::condition m_waiting_readers;
    int m_num_waiting_writers;
    int m_num_waiting_readers;
    boost::condition m_waiting_promotion;
    int m_num_waiting_promotion;
    int m_state;    // -1 = excl locked
                    // 0 = unlocked
                    // 1-> INT_MAX - shared locked
    rw_scheduling_policy m_sp;
    int m_readers_next; // For alternating priority policies, who goes next?

    void do_rdlock();
    void do_wrlock();
    void do_wrunlock();
    void do_rdunlock();
    bool do_try_wrlock();
    bool do_try_rdlock();
    bool do_timed_wrlock(const xtime &xt);
    bool do_timed_rdlock(const xtime &xt);
    bool do_try_promote_rdlock();
    void do_wakeups();
};

} // namespace detail

} // namespace thread

class BOOST_THREAD_DECL rw_mutex : private noncopyable
{
public:
    rw_mutex(rw_scheduling_policy sp) : m_impl(sp) { }
    ~rw_mutex() { }

    rw_scheduling_policy policy() const { return m_impl.m_sp; }

    friend class detail::thread::rw_lock_ops<rw_mutex>;
    typedef detail::thread::scoped_rw_lock<rw_mutex> scoped_rw_lock;
    typedef detail::thread::scoped_try_rw_lock<rw_mutex> scoped_try_rw_lock;

private:
    // Operations that will eventually be done only
    //   via lock types
    void do_wrlock();
    void do_rdlock();
    void do_wrunlock();
    void do_rdunlock();

    detail::thread::rw_mutex_impl<mutex> m_impl;
};

class BOOST_THREAD_DECL try_rw_mutex : private noncopyable
{
public:
    try_rw_mutex(rw_scheduling_policy sp) : m_impl(sp) { }
    ~try_rw_mutex() { }

    rw_scheduling_policy policy() const { return m_impl.m_sp; }

    friend class detail::thread::rw_lock_ops<try_rw_mutex>;
    typedef detail::thread::scoped_rw_lock<try_rw_mutex> scoped_rw_lock;
    typedef detail::thread::scoped_try_rw_lock<
        try_rw_mutex> scoped_try_rw_lock;

private:
    // Operations that will eventually be done only
    //   via lock types
    void do_wrlock();
    void do_rdlock();
    void do_wrunlock();
    void do_rdunlock();
    bool do_try_wrlock();
    bool do_try_rdlock();

    detail::thread::rw_mutex_impl<try_mutex> m_impl;
};

class BOOST_THREAD_DECL timed_rw_mutex : private noncopyable
{
public:
    timed_rw_mutex(rw_scheduling_policy sp) : m_impl(sp) { }
    ~timed_rw_mutex() { }

    rw_scheduling_policy policy() const { return m_impl.m_sp; }

    friend class detail::thread::rw_lock_ops<timed_rw_mutex>;
    typedef detail::thread::scoped_rw_lock<timed_rw_mutex> scoped_rw_lock;
    typedef detail::thread::scoped_try_rw_lock<
        timed_rw_mutex> scoped_try_rw_lock;
    typedef detail::thread::scoped_timed_rw_lock<
        timed_rw_mutex> scoped_timed_rw_lock;

private:
    // Operations that will eventually be done only
    //   via lock types
    void do_wrlock();
    void do_rdlock();
    void do_wrunlock();
    void do_rdunlock();
    bool do_try_wrlock();
    bool do_try_rdlock();
    bool do_timed_wrlock(const xtime &xt);
    bool do_timed_rdlock(const xtime &xt);

    detail::thread::rw_mutex_impl<timed_mutex> m_impl;
};

}   // namespace boost

#endif
