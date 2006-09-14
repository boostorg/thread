// Copyright (C) 2002-2003
// David Moore, William E. Kempf, Michael Glassford
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/*
PROBLEMS:

The algorithms are not exception safe. For instance, if conditon::wait()
or another call throws an exception, the lock state and other state data
are not appropriately adjusted.

A harder problem to fix is that, if a thread is killed while inside the
read-write mutex functions (for example, while waiting),
bad things happen.
*/

#include <boost/thread/detail/config.hpp>

#include <boost/assert.hpp>
#include <boost/thread/read_write_mutex.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/detail/workaround.hpp>

#if !defined(BOOST_NO_STRINGSTREAM)
#   include <sstream>
#endif

#ifdef BOOST_HAS_WINTHREADS
#   include <windows.h>
#   include <tchar.h>
#   include <stdio.h>

#   if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
        inline bool IsDebuggerPresent(void)
        {
            return false;
        }
#   endif
#   if !defined(OutputDebugString)
        inline void OutputDebugStringA(LPCSTR)
        {}
        inline void OutputDebugStringW(LPCWSTR)
        {}
        #define OutputDebugString(str)
#   endif

#   if defined(BOOST_READ_WRITE_MUTEX_USE_TRACE) && !defined(BOOST_NO_STRINGSTREAM)
        inline void DoTrace(
            const char* message,
            int state,
            int num_waiting_writers,
            int num_waiting_readers,
            bool state_waiting_promotion,
            int num_waking_writers,
            int num_waking_readers,
            int num_max_waking_writers,
            int num_max_waking_readers,
            bool readers_next
            )
        {
            std::ostringstream stream;
            stream
                << std::endl
                << "***** "
                << std::hex << GetCurrentThreadId() << std::dec << " "
                << message << " "
                << state << " "
                << "["
                << num_waiting_writers << " "
                << num_waking_writers << " "
                << num_max_waking_writers
                << "] ["
                << num_waiting_readers << " "
                << num_waking_readers << " "
                << num_max_waking_readers
                << "]" << " "
                << state_waiting_promotion << " "
                << readers_next
                << std::endl;
            ::OutputDebugStringA(stream.str().c_str());
        }

#       define BOOST_READ_WRITE_MUTEX_TRACE(message) \
            DoTrace(                                 \
                message,                             \
                m_state,                             \
                m_num_waiting_writers,               \
                m_num_waiting_readers,               \
                m_state_waiting_promotion,           \
                m_num_waking_writers,                \
                m_num_waking_readers,                \
                m_num_max_waking_writers,            \
                m_num_max_waking_readers,            \
                m_readers_next                       \
                )
#   endif
#endif

#if !defined(BOOST_READ_WRITE_MUTEX_TRACE)
#   define BOOST_READ_WRITE_MUTEX_TRACE(message)
#endif

#if defined(BOOST_ASSERT)
#   define BOOST_ASSERT_ELSE(expr) if ((BOOST_ASSERT(expr)), true)
#else
#   define BOOST_ASSERT_ELSE(expr) if (true)
#endif

//The following macro checks for invalid loop conditions
//by checking for wait loops that loop more than once.
//Please note that this does not necessarily indicate any
//kind of error; there are several valid reasons for
//a wait loop to loop more than once. For instance:
//1) if a condition signals a spurious wakeup,
//   it should wait again;
//2) if a several waiting threads (e.g. readers) are 
//   notified, and the first to wake changes conditions 
//   so that the others should no longer wake (e.g.
//   by promoting itself to a writer), the wait loops
//   of the other threads, when they wake, will loop again
//   and they will wait again.
//For this reason, the BOOST_ASSERT_LOOP_COUNT is only
//enabled when specifically requested by #defining
//the BOOST_READ_WRITE_MUTEX_TEST_LOOP_COUNTS macro.

#if defined(BOOST_READ_WRITE_MUTEX_TEST_LOOP_COUNTS)
#   define BOOST_DEFINE_LOOP_COUNT int loop_count = 0;
#   define BOOST_ASSERT_LOOP_COUNT() BOOST_ASSERT(++loop_count == 1)
#else
#   define BOOST_DEFINE_LOOP_COUNT do {} while(false);
#   define BOOST_ASSERT_LOOP_COUNT() do {} while(false)
#endif

bool boost_error(char const* expr, char const* func, char const* file, long line)
{
    #if WINVER
        using namespace std;

        #ifndef ELEMENTS
        #define ELEMENTS(a) (sizeof(a)/sizeof(*(a)))
        #endif

        TCHAR message[200];
        _sntprintf(message,ELEMENTS(message),TEXT("Assertion failed (func=%s, file=%s, line=%d): %s"), func, file, line, expr);

        ::OutputDebugString(message);

        if(::IsDebuggerPresent())
            ::DebugBreak();
    #endif

    return false;
}

namespace boost {
namespace detail {
namespace thread {

//----------------------------------------
    
inline bool valid_lock_state(int state)
{
    return (state >= 0) || (state == -1);
}

inline bool valid_read_write_locked(int state)
{
    return state != 0;
}

inline bool valid_read_locked(int state)
{
    return state > 0;
}

inline bool valid_read_lockable(int state)
{
    return state >= 0;
}

inline bool valid_write_locked(int state)
{
    return state == -1;
}

inline bool valid_write_lockable(int state)
{
    return state == 0;
}

inline bool valid_promotable(int state)
{
    return state == 1;
}

inline bool valid_unlocked(int state)
{
    return state == 0;
}

//----------------------------------------

class adjust_state
{
public:

    adjust_state(bool& state, bool adjust = true)
        : state_(state)
        , adjust_(adjust)
    {
        if (adjust_)
            state_ = true;
    }

    ~adjust_state(void)
    {
        adjust_now();
    }

    void set_adjust(bool adjust)
    {
        adjust_ = adjust;
    }

    void adjust_now(void)
    {
        if (adjust_)
        {
            BOOST_ASSERT(state_);
            state_ = false;
        }
        else
        {
            BOOST_ASSERT(!state_);
        }
        adjust_ = false;
    }

private:

    bool& state_;
    bool adjust_;
};

//----------------------------------------

class adjust_count
{
public:

    adjust_count(int& count, bool adjust = true)
        : count_(count)
        , adjust_(adjust)
    {
        if (adjust_)
            ++count_;
    }

    ~adjust_count(void)
    {
        adjust_now();
    }

    void set_adjust(bool adjust)
    {
        adjust_ = adjust;
    }

    void adjust_now(void)
    {
        if (adjust_)
        {
            BOOST_ASSERT(count_ > 0);
            if (count_ > 0)
                --count_;
        }
        else
        {
            BOOST_ASSERT(count_ >= 0);
        }
        adjust_ = false;
    }

private:

    int& count_;
    bool adjust_;
};

//----------------------------------------

/*
Because of the possibility that threads that call
timed_wait may timeout instead of waking, even after
they have been notified, the counters m_num_waking_writers
and m_num_waking_readers cannot be exact, but rather keep
track of the minimum number of writers and readers (respectively)
that are waking. For this reason, adjust_count may decrement
too many times. The max_count mechanism is an attempt to
keep track of the maximum as well.
*/

class adjust_dual_count
{
public:

    adjust_dual_count(int& count, int& max_count, bool adjust = true)
        : count_(count)
        , max_count_(max_count)
        , adjust_(adjust)
    {
        BOOST_ASSERT(&max_count_ != &count_);
        BOOST_ASSERT(max_count_ >= count_);

        if (adjust_)
        {
            ++count_;
            ++max_count_;
        }
    }

    ~adjust_dual_count(void)
    {
        adjust_now();
    }

    void set_adjust(bool adjust)
    {
        adjust_ = adjust;
    }

    void adjust_now(void)
    {
        BOOST_ASSERT(max_count_ >= count_);

        if (adjust_)
        {
            BOOST_ASSERT(max_count_ > 0);
            if (count_ > 0)
                --count_;
            if (max_count_ > 0)
                --max_count_;
        }
        else
        {
            BOOST_ASSERT(max_count_ >= 0);
        }
        adjust_ = false;
    }

private:

    int& count_;
    int& max_count_;
    bool adjust_;
};

//----------------------------------------

template<typename Mutex>
read_write_mutex_impl<Mutex>::read_write_mutex_impl(read_write_scheduling_policy::read_write_scheduling_policy_enum sp)
    : m_sp(sp)
    , m_state(0)
    , m_num_waiting_writers(0)
    , m_num_waiting_readers(0)
    , m_state_waiting_promotion(false)
    , m_num_waking_writers(0)
    , m_num_waking_readers(0)
    , m_num_max_waking_writers(0)
    , m_num_max_waking_readers(0)
    , m_readers_next(true) 
{}

#if !BOOST_WORKAROUND(__BORLANDC__, <= 0x564)
template<typename Mutex>
read_write_mutex_impl<Mutex>::~read_write_mutex_impl()
{
    BOOST_ASSERT(valid_unlocked(m_state));
    
    BOOST_ASSERT(m_num_waiting_writers == 0);
    BOOST_ASSERT(m_num_waiting_readers == 0);
    BOOST_ASSERT(!m_state_waiting_promotion);

    BOOST_ASSERT(m_num_waking_writers == 0);
    BOOST_ASSERT(m_num_max_waking_writers == 0);
    BOOST_ASSERT(m_num_waking_readers == 0);
    BOOST_ASSERT(m_num_max_waking_readers == 0);
}
#endif

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_read_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_read_lock() enter");
    BOOST_ASSERT(valid_lock_state(m_state));

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while write-locked

        BOOST_DEFINE_LOOP_COUNT;
        adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
        while (m_state == -1)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            m_waiting_readers.wait(l);
        };
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while write-locked or while writers are waiting

        BOOST_DEFINE_LOOP_COUNT;
        adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
//:     if (m_num_waiting_writers > 0 && m_num_waking_writers == 0)
//:         do_wake_one_writer();
        while (m_state == -1 || (m_num_waking_writers > 0) || (m_num_waiting_writers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            m_waiting_readers.wait(l);
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: wait while write-locked or while not readers' turn

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
        while (m_state == -1 || (m_num_waiting_writers > 0 && m_num_waking_readers == 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            m_waiting_readers.wait(l);
        }
    }

    //Obtain a read lock

    BOOST_ASSERT(valid_read_lockable(m_state));
    ++m_state;

    /*
    Set m_readers_next in the lock function rather than the 
    unlock function to prevent thread starvation that can happen,
    e.g., like this: if all writer threads demote themselves
    to reader threads before unlocking, they will unlock using 
    do_read_unlock() which will set m_readers_next = false;
    if there are enough writer threads, this will prevent any
    "true" reader threads from ever obtaining the lock.
    */

    m_readers_next = false;

    BOOST_ASSERT(valid_read_locked(m_state));
    BOOST_READ_WRITE_MUTEX_TRACE("do_read_lock() exit");
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_write_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_write_lock() enter");
    BOOST_ASSERT(valid_lock_state(m_state));

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while locked or while readers are waiting

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
//:     if (m_num_waiting_readers > 0 && m_num_waking_readers == 0)
//:         do_wake_all_readers();
        while (m_state != 0 || (m_num_waking_readers > 0) || (m_num_waiting_readers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            m_waiting_writers.wait(l);
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while locked

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state != 0)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            m_waiting_writers.wait(l);
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        if (m_sp == read_write_scheduling_policy::alternating_single_read && m_num_waiting_writers == 0)
            m_num_waking_readers = (m_readers_next && m_num_waking_readers > 0) ? 1 : 0;

        //Alternating priority: wait while locked or while not writers' turn

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state != 0 || (m_num_waking_readers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            m_waiting_writers.wait(l);
        }
    }

    //Obtain a write lock

    BOOST_ASSERT(valid_write_lockable(m_state));
    m_state = -1;

    //See note in read_write_mutex_impl<>::do_read_lock() as to why 
    //m_readers_next should be set here

    m_readers_next = true;

    BOOST_ASSERT(valid_write_locked(m_state));
    BOOST_READ_WRITE_MUTEX_TRACE("do_write_lock() exit");
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_read_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_lock_state(m_state));
    BOOST_READ_WRITE_MUTEX_TRACE("do_try_read_lock() enter");

    if (!l.locked())
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_try_read_lock() exit 1");
        return false;
    }

    bool fail;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: fail if write-locked
        fail = (m_state == -1);
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: fail if write-locked or if writers are waiting
        fail = (m_state == -1 || m_num_waiting_writers > 0);
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: fail if write-locked or if not readers' turn
        fail = (m_state == -1 || (m_num_waiting_writers > 0 && m_num_waking_readers == 0));
    }

    if (!fail)
    {
        //Obtain a read lock

        BOOST_ASSERT(valid_read_lockable(m_state));
        ++m_state;

        //See note in read_write_mutex_impl<>::do_read_lock() as to why 
        //m_readers_next should be set here

        m_readers_next = false;

        BOOST_ASSERT(valid_read_locked(m_state));
            //Should be read-locked
    }
    else
    {
        BOOST_ASSERT(valid_write_locked(m_state) || m_num_waiting_writers > 0);
            //Should be write-locked or
            //writer should be waiting
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_try_read_lock() exit 2");
    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_write_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_try_write_lock() enter");
    BOOST_ASSERT(valid_lock_state(m_state));

    if (!l.locked())
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_try_write_lock() exit 1");
        return false;
    }

    bool fail;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: fail if locked or if readers are waiting
        fail = (m_state != 0 || m_num_waiting_readers > 0);
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: fail if locked
        fail = (m_state != 0);
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: fail if locked or if not writers' turn
        fail = (m_state != 0 || m_num_waking_readers > 0);
    }

    if (!fail)
    {
        //Obtain a write lock

        BOOST_ASSERT(valid_write_lockable(m_state));
        m_state = -1;

        //See note in read_write_mutex_impl<>::do_read_lock() as to why 
        //m_readers_next should be set here

        m_readers_next = true;

        BOOST_ASSERT(valid_write_locked(m_state));
            //Should be write-locked
    }
    else
    {
        BOOST_ASSERT(valid_read_write_locked(m_state) || m_num_waiting_readers > 0);
            //Should be read-locked or write-locked, or
            //reader should be waiting
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_try_write_lock() exit 2");
    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_read_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_read_lock() enter");
    BOOST_ASSERT(valid_lock_state(m_state));

    if (!l.locked())
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_timed_read_lock() exit 1");
        return false;
    }

    bool fail = false;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while write-locked

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
        while (m_state == -1)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                ++m_num_max_waking_readers;
                fail = true; 
                break;
            }
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while write-locked or while writers are waiting

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
//:     if (m_num_waiting_writers > 0 && m_num_waking_writers == 0)
//:         do_wake_one_writer();
        while (m_state == -1 || (m_num_waking_writers > 0) || (m_num_waiting_writers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                ++m_num_max_waking_readers;
                fail = true; 
                break;
            }
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: wait while write-locked or while not readers' turn

        BOOST_DEFINE_LOOP_COUNT
        while (m_state == -1 || (m_num_waiting_writers > 0 && m_num_waking_readers == 0 && waker_exists()))
        {
            adjust_dual_count adjust_waking(m_num_waking_readers, m_num_max_waking_readers, false);
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_readers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                ++m_num_max_waking_readers;
                fail = true; 
                break;
            }
        }
    }

    if (!fail)
    {
        //Obtain a read lock

        BOOST_ASSERT(valid_read_lockable(m_state));
        ++m_state;

        //See note in read_write_mutex_impl<>::do_read_lock() as to why 
        //m_readers_next should be set here

        m_readers_next = false;

        BOOST_ASSERT(valid_read_locked(m_state));
            //Should be read-locked
    }
    else
    {
        //In case there is no thread with a lock that will 
        //call do_scheduling_impl() when it unlocks, call it ourselves
        do_scheduling_impl(scheduling_reason_timeout);
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_read_lock() exit 2");
    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_write_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_write_lock() enter");
    BOOST_ASSERT(valid_lock_state(m_state));

    if (!l.locked())
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_timed_write_lock() exit 1");
        return false;
    }

    bool fail = false;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while locked or while readers are waiting

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
//:     if (m_num_waiting_readers > 0 && m_num_waking_readers == 0)
//:         do_wake_all_readers();
        while (m_state != 0 || (m_num_waking_readers > 0) || (m_num_waiting_readers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                ++m_num_max_waking_writers;
                fail = true;
                break;
            }
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while locked

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state != 0)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                ++m_num_max_waking_writers;
                fail = true;
                break;
            }
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        if (m_sp == read_write_scheduling_policy::alternating_single_read && m_num_waiting_writers == 0)
            m_num_waking_readers = (m_readers_next && m_num_waking_readers > 0) ? 1 : 0;

        //Alternating priority: wait while locked or while not writers' turn

        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state != 0 || (m_num_waking_readers > 0 && waker_exists()))
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_waking.set_adjust(true);
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                ++m_num_max_waking_writers;
                fail = true;
                break;
            }
        }
    }

    if (!fail)
    {
        //Obtain a write lock

        BOOST_ASSERT(valid_write_lockable(m_state));
        m_state = -1;

        //See note in read_write_mutex_impl<>::do_read_lock() as to why 
        //m_readers_next should be set here

        m_readers_next = true;

        BOOST_ASSERT(valid_write_locked(m_state));
            //Should be write-locked
    }
    else
    {
        //In case there is no thread with a lock that will 
        //call do_scheduling_impl() when it unlocks, call it ourselves
        do_scheduling_impl(scheduling_reason_timeout);
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_write_lock() exit 2");
    return !fail;
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_read_unlock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_read_unlock() enter");
    BOOST_ASSERT(valid_read_locked(m_state));

    if (m_state > 0)
        --m_state;
    else //not read-locked
        throw lock_error();

    do_scheduling_impl(scheduling_reason_unlock);

    BOOST_ASSERT(valid_read_locked(m_state) || valid_unlocked(m_state));
    BOOST_READ_WRITE_MUTEX_TRACE("do_read_unlock() exit");
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_write_unlock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_write_unlock() enter");
    BOOST_ASSERT(valid_write_locked(m_state));

    if (m_state == -1)
        m_state = 0;
    else BOOST_ASSERT_ELSE(m_state >= 0)
        throw lock_error();      // Trying to release a reader-locked or unlocked mutex???

    do_scheduling_impl(scheduling_reason_unlock);

    BOOST_ASSERT(valid_unlocked(m_state));
    BOOST_READ_WRITE_MUTEX_TRACE("do_write_unlock() exit");
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_demote_to_read_lock_impl()
{
    BOOST_READ_WRITE_MUTEX_TRACE("do_demote_to_read_lock_impl() enter");
    BOOST_ASSERT(valid_write_locked(m_state));

    if (m_state == -1) 
    {
        //Convert from write lock to read lock
        m_state = 1;

        //If the conditions are right, release other readers

        do_scheduling_impl(scheduling_reason_demote);

        //Lock demoted
        BOOST_ASSERT(valid_read_locked(m_state));
        BOOST_READ_WRITE_MUTEX_TRACE("do_demote_to_read_lock_impl() exit 1");
        return true;
    }
    else BOOST_ASSERT_ELSE(m_state >= 0)
    {
        //Lock is read-locked or unlocked can't be demoted
        BOOST_READ_WRITE_MUTEX_TRACE("do_demote_to_read_lock_impl() exit 2");
        throw lock_error();
        return false;
    }
    return false; //Eliminate warnings on some compilers
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_demote_to_read_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_write_locked(m_state));

    do_demote_to_read_lock_impl();
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_demote_to_read_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_write_locked(m_state));

    if (!l.locked())
        return false;
    else //(l.locked())
        return do_demote_to_read_lock_impl();
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_demote_to_read_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_ASSERT(valid_write_locked(m_state));

    if (!l.locked())
        return false;
    else //(l.locked())
        return do_demote_to_read_lock_impl();
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_promote_to_write_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_promote_to_write_lock() enter");
    BOOST_ASSERT(valid_read_locked(m_state));

    if (m_state == 1)
    {
        //Convert from read lock to write lock
        m_state = -1;

        //Lock promoted
        BOOST_ASSERT(valid_write_locked(m_state));
    }
    else if (m_state <= 0)
    {
        //Lock is write-locked or unlocked can't be promoted
        throw lock_error();
    }
    else if (m_state_waiting_promotion)
    {
        //Someone else is already trying to promote. Avoid deadlock by throwing exception.
        throw lock_error();
    }
    else BOOST_ASSERT_ELSE(m_state > 1 && !m_state_waiting_promotion)
    {
        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state > 1)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_state adjust_waiting_promotion(m_state_waiting_promotion);
            adjust_waking.set_adjust(true);
            m_waiting_promotion.wait(l);
        }

        BOOST_ASSERT(m_num_waiting_writers >= 0);
        BOOST_ASSERT(valid_promotable(m_state));

        //Convert from read lock to write lock
        m_state = -1;
        
        //Lock promoted
        BOOST_ASSERT(valid_write_locked(m_state));
    }
    BOOST_READ_WRITE_MUTEX_TRACE("do_promote_to_write_lock() exit");
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_promote_to_write_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_READ_WRITE_MUTEX_TRACE("do_try_promote_to_write_lock() enter");
    BOOST_ASSERT(valid_read_locked(m_state));

    bool result;

    if (!l.locked())
        result = false;
    else
    {
        if (m_state == 1)
        {
            //Convert from read lock to write lock
            m_state = -1;

            //Lock promoted
            BOOST_ASSERT(valid_write_locked(m_state));
            result = true;
        }
        else if (m_state <= 0)
        {
            //Lock is write-locked or unlocked can't be promoted
            result = false;
        }
        else if (m_state_waiting_promotion)
        {
            //Someone else is already trying to promote. Avoid deadlock by returning false.
            result = false;
        }
        else BOOST_ASSERT_ELSE(m_state > 1 && !m_state_waiting_promotion)
        {
            //There are other readers, so we can't promote
            result = false;
        }
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_try_promote_to_write_lock() exit");
    return result;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_promote_to_write_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_promote_to_write_lock() enter");
    BOOST_ASSERT(valid_read_locked(m_state));

    if (!l.locked())
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_timed_promote_to_write_lock( exit 1");
        return false;
    }

    bool fail = false;

    if (m_state == 1)
    {
        fail = false;
    }
    else if (m_state <= 0)
    {
        //Lock is not read-locked and can't be promoted
        fail = true;
    }
    else if (m_state_waiting_promotion)
    {
        //Someone else is already trying to promote. Avoid deadlock by returning false.
        fail = true;
    }
    else BOOST_ASSERT_ELSE(m_state > 1 && !m_state_waiting_promotion)
    {   
        BOOST_DEFINE_LOOP_COUNT
        adjust_dual_count adjust_waking(m_num_waking_writers, m_num_max_waking_writers, false);
        while (m_state > 1)
        {
            BOOST_ASSERT_LOOP_COUNT(); //See note at BOOST_ASSERT_LOOP_COUNT definition above
            BOOST_ASSERT(waker_exists()); //There should be someone to wake us up
            adjust_count adjust_waiting(m_num_waiting_writers);
            adjust_state adjust_waiting_promotion(m_state_waiting_promotion);
            adjust_waking.set_adjust(true);
            if (!m_waiting_promotion.timed_wait(l, xt))
            {
                ++m_num_max_waking_writers;
                fail = true;
                break;
            }
        }
    }

    if (!fail)
    {
        //Convert from read lock to write lock

        BOOST_ASSERT(m_num_waiting_writers >= 0);
        BOOST_ASSERT(valid_promotable(m_state));

        m_state = -1;
        
        //Lock promoted

        BOOST_ASSERT(valid_write_locked(m_state));
    }
    else
    {
        //In case there is no thread with a lock that will 
        //call do_scheduling_impl() when it unlocks, call it ourselves
        do_scheduling_impl(scheduling_reason_timeout);
    }

    BOOST_READ_WRITE_MUTEX_TRACE("do_timed_promote_to_write_lock( exit 2");
    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::locked()
{
    int state = m_state;
    BOOST_ASSERT(valid_lock_state(state));

    return state != 0;
}

template<typename Mutex>
read_write_lock_state::read_write_lock_state_enum read_write_mutex_impl<Mutex>::state()
{
    int state = m_state;
    BOOST_ASSERT(valid_lock_state(state));

    if (state > 0)
    {
        BOOST_ASSERT(valid_read_locked(state));
        return read_write_lock_state::read_locked;
    }
    else if (state == -1)
    {
        BOOST_ASSERT(valid_write_locked(state));
        return read_write_lock_state::write_locked;
    }
    else BOOST_ASSERT_ELSE(state == 0)
        return read_write_lock_state::unlocked;
    return read_write_lock_state::unlocked; //Eliminate warnings on some compilers
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_scheduling_impl(const scheduling_reason reason)
{
    switch(reason)
    {
        case scheduling_reason_unlock:
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl(): scheduling_reason_unlock");

            //A thread just unlocked, so there can be no existing write lock.
            //There may still be read locks, however.

            BOOST_ASSERT(valid_read_locked(m_state) || valid_unlocked(m_state));

            if (m_state_waiting_promotion)
            {
                //If a thread is waiting for promotion,
                //it must have a read lock.

                BOOST_ASSERT(valid_read_locked(m_state));
            }
        }
        break;

        case scheduling_reason_timeout:
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl(): scheduling_reason_timeout");

            //A thread waiting for a lock just timed out, so the
            //lock could be in any state (read locked, write locked, unlocked).

            BOOST_ASSERT(valid_lock_state(m_state));

            if (m_state_waiting_promotion)
            {
                //If a thread is waiting for promotion,
                //it must have a read lock.

                BOOST_ASSERT(valid_read_locked(m_state));
            }
        }
        break;

        case scheduling_reason_demote:
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl(): scheduling_reason_demote");

            //A write lock has just converted its state to a read lock;
            //since a write-locked thread has an exclusive lock,
            //and no other thread has yet been allowed to obtain
            //a read lock, the state should indicate that there is 
            //exactly one reader.

            BOOST_ASSERT(m_state == 1);

            //No thread should be waiting for promotion because to do
            //so it would first have to obtain a read lock, which
            //is impossible because this thread had a write lock.

            BOOST_ASSERT(!m_state_waiting_promotion);
        }
        break;

        default:
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl(): scheduling_reason_none");

            throw lock_error();
            return; //eliminate warnings on some compilers
        }
        break;
    };

    bool woken;

    if (m_num_waiting_writers > 0 && m_num_waiting_readers > 0)
    {
        //Both readers and writers waiting: use scheduling policy

        if (m_sp == read_write_scheduling_policy::reader_priority)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 1: writers & readers, reader_priority");
            woken = do_wake_all_readers();
            if (woken)
                m_num_waking_writers = m_num_max_waking_writers = 0; //shut down any waking writers
        }
        else if (m_sp == read_write_scheduling_policy::writer_priority)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl(): writers & readers, writer_priority");
            woken = do_wake_writer();
            if (woken)
                m_num_waking_readers  = m_num_max_waking_readers = 0; //shut down any waking readers
        }
        else if (m_sp == read_write_scheduling_policy::alternating_single_read)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 2: writers & readers, alternating_single_read");
            if (m_readers_next)
            {
                if (m_num_waking_writers == 0)
                    woken = do_wake_one_reader();
                else
                    woken = false;
            }
            else
            {
                if (m_num_waking_readers == 0)
                    woken = do_wake_writer();
                else
                    woken = false;
            }
        }
        else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_many_reads)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 3: writers & readers, alternating_many_reads");
            if (m_readers_next)
            {
                if (m_num_waking_writers == 0)
                    woken = do_wake_all_readers();
                else
                    woken = false;
            }
            else
            {
                if (m_num_waking_readers == 0)
                    woken = do_wake_writer();
                else
                    woken = false;
            }
        }
    }
    else if (m_num_waiting_writers > 0)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 4: writers only");
        //Only writers waiting--scheduling policy doesn't matter
        woken = do_wake_writer();
    }
    else if (m_num_waiting_readers > 0)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 5: readers only");
        //Only readers waiting--scheduling policy doesn't matter    
        woken = do_wake_all_readers();
    }
    else
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_scheduling_impl() 6: no writers or readers");
        woken = false;
    }

    BOOST_ASSERT(
        woken 
        || (m_state == -1) || (m_state > (m_state_waiting_promotion ? 1 : 0))
        || (m_num_waking_writers + m_num_waking_readers > 0)
        || (m_num_waiting_writers + m_num_waiting_readers == 0)
        );
        //Ensure that we woke a thread,
        //that another besides the current thread is already awake to wake others when it's done, 
        //that another besides the current thread will wake and can wake others when it's done,
        //or that no other threads are waiting and so none remain to be woken
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_wake_one_reader(void)
{
    if (m_state == -1)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_one_reader() 1: don't wake, write locked");

        //If write-locked, don't bother waking a reader
    }
    else if (m_num_waking_readers > 0)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_one_reader() 2: don't wake, nothing to wake");

        //If a reader is already waking,
        //don't bother waking another
        //(since we only want one)
    }
    else if (m_num_waiting_readers > 0)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_one_reader() 3: wake");

        //Wake a reader
        BOOST_ASSERT(valid_read_lockable(m_state));
        m_num_waking_readers = m_num_max_waking_readers = 1;
        m_waiting_readers.notify_one();
        return true;
    }
    else
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_one_reader() 4: don't wake, nothing to wake");
    }

    return false;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_wake_all_readers(void)
{
    if (m_state == -1)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_all_readers() 1: don't wake, write locked");

        //If write-locked, don't bother waking readers
    }
    else if (m_num_waiting_readers > 0)
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_all_readers() 2: wake");

        //Wake readers
        BOOST_ASSERT(valid_read_lockable(m_state));
        m_num_waking_readers = m_num_max_waking_readers = m_num_waiting_readers;
        m_waiting_readers.notify_all();
        return true;
    }
    else
    {
        BOOST_READ_WRITE_MUTEX_TRACE("do_wake_all_readers() 3: don't wake, nothing to wake");
    }

    return false;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_wake_writer(void)
{
    if (m_state_waiting_promotion)
    {
        //If a reader is waiting for promotion, promote it
        //(it holds a read lock until it is promoted,
        //so it's not possible to wake a normal writer).

        if (m_state == -1 || m_state > 1)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 1: waiting promotion: don't wake, still locked");

            //If write-locked, or if read-locked by
            //readers other than the thread waiting
            //for promotion, don't bother waking it
        }
        else if (m_num_waking_writers > 0)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 2: waiting promotion: don't wake, writer already waking");

            //If a writer is already waking,
            //don't bother waking another
            //(since only one at a time can wake anyway)
        }
        else if (m_num_waiting_writers > 0)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 3: waiting promotion: wake");

            //Wake the thread waiting for promotion
            BOOST_ASSERT(valid_promotable(m_state));
            m_num_waking_writers = m_num_max_waking_writers = 1;
            m_waiting_promotion.notify_one();
            return true;
        }
        else
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 4: waiting promotion: don't wake, no writers to wake");
        }
    }
    else
    {
        if (m_state != 0)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 5: don't wake, still locked");

            //If locked, don't bother waking a writer
        }
        else if (m_num_waking_writers > 0)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 6: don't wake, writer already waking");

            //If a writer is already waking,
            //don't bother waking another
            //(since only one at a time can wake anyway)
        }
        else if (m_num_waiting_writers > 0)
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 7: wake");

            //Wake a writer
            BOOST_ASSERT(valid_write_lockable(m_state));
            m_num_waking_writers = m_num_max_waking_writers = 1;
            m_waiting_writers.notify_one();
            return true;
        }
        else
        {
            BOOST_READ_WRITE_MUTEX_TRACE("do_wake_writer() 8: don't wake, no writers to wake");
        }
    }

    return false;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::waker_exists(void)
{
    //Is there a "live" thread (one that is awake or about to wake
    //that will be able to wake up a thread about to go to sleep?
    return valid_read_write_locked(m_state) || (m_num_waking_writers + m_num_waking_readers > 0);
}

    }   // namespace thread
    }   // namespace detail

read_write_mutex::read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp)
    : m_impl(sp)
{}

read_write_mutex::~read_write_mutex() 
{}

void read_write_mutex::do_read_lock()
{
    m_impl.do_read_lock();
}

void read_write_mutex::do_write_lock()
{
    m_impl.do_write_lock();
}

void read_write_mutex::do_read_unlock()
{
    m_impl.do_read_unlock();
}

void read_write_mutex::do_write_unlock()
{
    m_impl.do_write_unlock();
}

void read_write_mutex::do_demote_to_read_lock()
{
    m_impl.do_demote_to_read_lock();
}

void read_write_mutex::do_promote_to_write_lock()
{
    m_impl.do_promote_to_write_lock();
}

bool read_write_mutex::locked()
{
    return m_impl.locked();
}

read_write_lock_state::read_write_lock_state_enum read_write_mutex::state()
{
    return m_impl.state();
}

try_read_write_mutex::try_read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp) 
    : m_impl(sp)
{}

try_read_write_mutex::~try_read_write_mutex()
{}

void try_read_write_mutex::do_read_lock()
{
    m_impl.do_read_lock();
}

void try_read_write_mutex::do_write_lock()
{
    m_impl.do_write_lock();

}

void try_read_write_mutex::do_write_unlock()
{
    m_impl.do_write_unlock();
}

void try_read_write_mutex::do_read_unlock()
{
    m_impl.do_read_unlock();
}

bool try_read_write_mutex::do_try_read_lock()
{
    return m_impl.do_try_read_lock();
}

bool try_read_write_mutex::do_try_write_lock()
{
    return m_impl.do_try_write_lock();
}

void try_read_write_mutex::do_demote_to_read_lock()
{
    m_impl.do_demote_to_read_lock();
}

bool try_read_write_mutex::do_try_demote_to_read_lock()
{
    return m_impl.do_try_demote_to_read_lock();
}

void try_read_write_mutex::do_promote_to_write_lock()
{
    m_impl.do_promote_to_write_lock();
}

bool try_read_write_mutex::do_try_promote_to_write_lock()
{
    return m_impl.do_try_promote_to_write_lock();
}

bool try_read_write_mutex::locked()
{
    return m_impl.locked();
}

read_write_lock_state::read_write_lock_state_enum try_read_write_mutex::state()
{
    return m_impl.state();
}

timed_read_write_mutex::timed_read_write_mutex(read_write_scheduling_policy::read_write_scheduling_policy_enum sp) 
    : m_impl(sp)
{}

timed_read_write_mutex::~timed_read_write_mutex()
{}

void timed_read_write_mutex::do_read_lock()
{
    m_impl.do_read_lock();
}

void timed_read_write_mutex::do_write_lock()
{
    m_impl.do_write_lock();

}

void timed_read_write_mutex::do_read_unlock()
{
    m_impl.do_read_unlock();
}

void timed_read_write_mutex::do_write_unlock()
{
    m_impl.do_write_unlock();
}

bool timed_read_write_mutex::do_try_read_lock()
{
    return m_impl.do_try_read_lock();
}

bool timed_read_write_mutex::do_try_write_lock()
{
    return m_impl.do_try_write_lock();
}

bool timed_read_write_mutex::do_timed_read_lock(const xtime &xt)
{
    return m_impl.do_timed_read_lock(xt);
}

bool timed_read_write_mutex::do_timed_write_lock(const xtime &xt)
{
    return m_impl.do_timed_write_lock(xt);
}

void timed_read_write_mutex::do_demote_to_read_lock()
{
    m_impl.do_demote_to_read_lock();
}

bool timed_read_write_mutex::do_try_demote_to_read_lock()
{
    return m_impl.do_try_demote_to_read_lock();
}

bool timed_read_write_mutex::do_timed_demote_to_read_lock(const xtime &xt)
{
    return m_impl.do_timed_demote_to_read_lock(xt);
}

void timed_read_write_mutex::do_promote_to_write_lock()
{
    m_impl.do_promote_to_write_lock();
}

bool timed_read_write_mutex::do_try_promote_to_write_lock()
{
    return m_impl.do_try_promote_to_write_lock();
}

bool timed_read_write_mutex::do_timed_promote_to_write_lock(const xtime &xt)
{
    return m_impl.do_timed_promote_to_write_lock(xt);
}

bool timed_read_write_mutex::locked()
{
    return m_impl.locked();
}

read_write_lock_state::read_write_lock_state_enum timed_read_write_mutex::state()
{
    return m_impl.state();
}

#if !defined(NDEBUG)
    //Explicit instantiations of read/write locks to catch syntax errors in templates

    template class boost::detail::thread::scoped_read_write_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_read_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_read_write_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_try_read_write_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_try_read_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_try_read_write_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_timed_read_write_lock<read_write_mutex>;
    //template class boost::detail::thread::scoped_timed_read_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_timed_read_write_lock<timed_read_write_mutex>;

    //Explicit instantiations of read locks to catch syntax errors in templates

    template class boost::detail::thread::scoped_read_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_read_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_read_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_try_read_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_try_read_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_try_read_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_timed_read_lock<read_write_mutex>;
    //template class boost::detail::thread::scoped_timed_read_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_timed_read_lock<timed_read_write_mutex>;

    //Explicit instantiations of write locks to catch syntax errors in templates

    template class boost::detail::thread::scoped_write_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_write_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_try_write_lock<read_write_mutex>;
    template class boost::detail::thread::scoped_try_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_try_write_lock<timed_read_write_mutex>;

    //template class boost::detail::thread::scoped_timed_write_lock<read_write_mutex>;
    //template class boost::detail::thread::scoped_timed_write_lock<try_read_write_mutex>;
    template class boost::detail::thread::scoped_timed_write_lock<timed_read_write_mutex>;
#endif
} // namespace boost

// Change Log:
//  10 Mar 02 
//      Original version.
//   4 May 04 GlassfordM
//      For additional changes, see read_write_mutex.hpp.
//      Add many assertions to test validity of mutex state and operations.
//      Rework scheduling algorithm due to addition of lock promotion and 
//         demotion.
//      Add explicit template instantiations to catch syntax errors 
//         in templates.
