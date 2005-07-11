// Copyright (C)  2002-2003
// David Moore, William E. Kempf, Michael Glassford
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

#ifndef BOOST_READ_WRITE_MUTEX_JDM030602_HPP
#define BOOST_READ_WRITE_MUTEX_JDM030602_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/detail/lock.hpp>
#include <boost/thread/detail/read_write_lock.hpp>
#include <boost/thread/condition.hpp>

namespace boost {

namespace read_write_scheduling_policy {
    enum read_write_scheduling_policy_enum
    {
        writer_priority,               //Prefer writers; can starve readers
        reader_priority,               //Prefer readers; can starve writers
        alternating_many_reads,        //Alternate readers and writers; before a writer, release all queued readers 
        alternating_single_read        //Alternate readers and writers; before a writer, release only one queued reader
    };
} // namespace read_write_scheduling_policy

namespace detail {

namespace thread {

// Shared implementation construct for explicit Scheduling Policies
// This implementation is susceptible to self-deadlock, though....
template<typename Mutex>
struct read_write_mutex_impl
{
    typedef Mutex mutex_type;
    typedef detail::thread::scoped_lock<Mutex> scoped_lock;
    typedef detail::thread::scoped_try_lock<Mutex> scoped_try_lock;
    typedef detail::thread::scoped_timed_lock<Mutex> scoped_timed_lock;

    read_write_mutex_impl(read_write_scheduling_policy::read_write_scheduling_policy_enum sp);
#if !BOOST_WORKAROUND(__BORLANDC__,<= 0x564)
    ~read_write_mutex_impl();
#endif

    Mutex m_prot;

    const read_write_scheduling_policy::read_write_scheduling_policy_enum m_sp;
    int m_state; //-1 = write lock; 0 = unlocked; >0 = read locked

    boost::condition m_waiting_writers;
    boost::condition m_waiting_readers;
    boost::condition m_waiting_promotion;
    int m_num_waiting_writers;
    int m_num_waiting_readers;
    bool m_state_waiting_promotion;

    int m_num_waking_writers;
    int m_num_waking_readers;
    int m_num_max_waking_writers; //Debug only
    int m_num_max_waking_readers; //Debug only

    bool m_readers_next;

    void do_read_lock();
    void do_write_lock();
    void do_write_unlock();
    void do_read_unlock();
    bool do_try_write_lock();
    bool do_try_read_lock();
    bool do_timed_write_lock(const xtime &xt);
    bool do_timed_read_lock(const xtime &xt);

    void do_demote_to_read_lock();
    bool do_try_demote_to_read_lock();
    bool do_timed_demote_to_read_lock(const xtime &xt);

    void do_promote_to_write_lock();
    bool do_try_promote_to_write_lock();
    bool do_timed_promote_to_write_lock(const xtime &xt);

    bool locked();
    read_write_lock_state::read_write_lock_state_enum state();

private:

    bool do_demote_to_read_lock_impl();

    enum scheduling_reason
    {
        scheduling_reason_unlock,
        scheduling_reason_timeout,
        scheduling_reason_demote
    };

    void do_scheduling_impl(const scheduling_reason reason);
    bool do_wake_one_reader(void);
    bool do_wake_all_readers(void);
    bool do_wake_writer(void);
    bool waker_exists(void);
};

} // namespace detail

} // namespace thread

class BOOST_THREAD_DECL read_write_mutex : private noncopyable
{
public:

    read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp);
    ~read_write_mutex();

    read_write_scheduling_policy::read_write_scheduling_policy_enum policy() const { return m_impl.m_sp; }

    friend class detail::thread::read_write_lock_ops<read_write_mutex>;

    typedef detail::thread::scoped_read_write_lock<
        read_write_mutex> scoped_read_write_lock;

    typedef detail::thread::scoped_read_lock<
        read_write_mutex> scoped_read_lock;

    typedef detail::thread::scoped_write_lock<
        read_write_mutex> scoped_write_lock;

private:

    // Operations that will eventually be done only
    //   via lock types
    void do_write_lock();
    void do_read_lock();
    void do_write_unlock();
    void do_read_unlock();

    void do_demote_to_read_lock();

    void do_promote_to_write_lock();

    bool locked();
    read_write_lock_state::read_write_lock_state_enum state();

    detail::thread::read_write_mutex_impl<mutex> m_impl; 
};

class BOOST_THREAD_DECL try_read_write_mutex : private noncopyable
{
public:

    try_read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp);
    ~try_read_write_mutex();

    read_write_scheduling_policy::read_write_scheduling_policy_enum policy() const { return m_impl.m_sp; }

    friend class detail::thread::read_write_lock_ops<try_read_write_mutex>;

    typedef detail::thread::scoped_read_write_lock<
        try_read_write_mutex> scoped_read_write_lock;
    typedef detail::thread::scoped_try_read_write_lock<
        try_read_write_mutex> scoped_try_read_write_lock;

    typedef detail::thread::scoped_read_lock<
        try_read_write_mutex> scoped_read_lock;
    typedef detail::thread::scoped_try_read_lock<
        try_read_write_mutex> scoped_try_read_lock;

    typedef detail::thread::scoped_write_lock<
        try_read_write_mutex> scoped_write_lock;
    typedef detail::thread::scoped_try_write_lock<
        try_read_write_mutex> scoped_try_write_lock;

private:

    // Operations that will eventually be done only
    //   via lock types
    void do_write_lock();
    void do_read_lock();
    void do_write_unlock();
    void do_read_unlock();
    bool do_try_write_lock();
    bool do_try_read_lock();


    void do_demote_to_read_lock();
    bool do_try_demote_to_read_lock();

    void do_promote_to_write_lock();
    bool do_try_promote_to_write_lock();

    bool locked();
    read_write_lock_state::read_write_lock_state_enum state();

    detail::thread::read_write_mutex_impl<try_mutex> m_impl; 
};

class BOOST_THREAD_DECL timed_read_write_mutex : private noncopyable
{
public:

    timed_read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp);
    ~timed_read_write_mutex();

    read_write_scheduling_policy::read_write_scheduling_policy_enum policy() const { return m_impl.m_sp; }

    friend class detail::thread::read_write_lock_ops<timed_read_write_mutex>;

    typedef detail::thread::scoped_read_write_lock<
        timed_read_write_mutex> scoped_read_write_lock;
    typedef detail::thread::scoped_try_read_write_lock<
        timed_read_write_mutex> scoped_try_read_write_lock;
    typedef detail::thread::scoped_timed_read_write_lock<
        timed_read_write_mutex> scoped_timed_read_write_lock;

    typedef detail::thread::scoped_read_lock<
        timed_read_write_mutex> scoped_read_lock;
    typedef detail::thread::scoped_try_read_lock<
        timed_read_write_mutex> scoped_try_read_lock;
    typedef detail::thread::scoped_timed_read_lock<
        timed_read_write_mutex> scoped_timed_read_lock;

    typedef detail::thread::scoped_write_lock<
        timed_read_write_mutex> scoped_write_lock;
    typedef detail::thread::scoped_try_write_lock<
        timed_read_write_mutex> scoped_try_write_lock;
    typedef detail::thread::scoped_timed_write_lock<
        timed_read_write_mutex> scoped_timed_write_lock;

private:

    // Operations that will eventually be done only
    //   via lock types
    void do_write_lock();
    void do_read_lock();
    void do_write_unlock();
    void do_read_unlock();
    bool do_try_write_lock();
    bool do_try_read_lock();
    bool do_timed_write_lock(const xtime &xt);
    bool do_timed_read_lock(const xtime &xt);
    
    void do_demote_to_read_lock();
    bool do_try_demote_to_read_lock();
    bool do_timed_demote_to_read_lock(const xtime &xt);

    void do_promote_to_write_lock();
    bool do_try_promote_to_write_lock();
    bool do_timed_promote_to_write_lock(const xtime &xt);

    bool locked();
    read_write_lock_state::read_write_lock_state_enum state();

    detail::thread::read_write_mutex_impl<timed_mutex> m_impl; 
};

}    // namespace boost

#endif

// Change Log:
//  10 Mar 02 
//      Original version.
//   4 May 04 GlassfordM
//      Implement lock promotion and demotion.
//      Add locked() and state() member functions for debugging
//         (should these be made public?).
//      Rename to improve consistency and eliminate abbreviations:
//          Use "read" and "write" instead of "shared" and "exclusive".
//          Change "rd" to "read", "wr" to "write", "rw" to "read_write".
//      Add mutex_type typdef.
