// Copyright (C) 2002-2003
// David Moore, William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/detail/config.hpp>

#include <boost/assert.hpp>
#include <boost/thread/read_write_mutex.hpp>
#include <boost/thread/xtime.hpp>

#ifdef BOOST_HAS_WINTHREADS
    #include <windows.h>
    #include <tchar.h>

    #if !((_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400))
        inline bool IsDebuggerPresent(void)
        {
            return false;
        }
    #endif
#endif

#if defined(BOOST_ASSERT)
#   define BOOST_ASSERT_ELSE(expr) if ((BOOST_ASSERT(expr)), false) {} else
#else
#   define BOOST_ASSERT_ELSE(expr) if (false) {} else
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

inline bool valid_lock(int state)
{
    return (state >= 0) || (state == -1);
}

inline bool valid_read_write_lock(int state)
{
    return state != 0;
}

inline bool valid_read_lock(int state)
{
    return state > 0;
}

inline bool valid_read_lockable(int state)
{
    return state >= 0;
}

inline bool valid_write_lock(int state)
{
    return state == -1;
}

inline bool valid_write_lockable(int state)
{
    return state == 0;
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_read_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_lock(m_state));

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while write-locked

        int loop_count = 0;
        while (m_state == -1)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            m_waiting_readers.wait(l);
            --m_num_waiting_readers;
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while write-locked or while writers are waiting

        int loop_count = 0;
        while (m_state == -1 || m_num_waiting_writers > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            m_waiting_readers.wait(l);
            --m_num_waiting_readers;
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: wait while write-locked or while not readers' turn

        int loop_count = 0;
        while (m_state == -1 || m_num_readers_to_wake == 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            m_waiting_readers.wait(l);
            --m_num_waiting_readers;
        }

        BOOST_ASSERT(m_num_readers_to_wake > 0);
        --m_num_readers_to_wake;
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

    BOOST_ASSERT(valid_read_lock(m_state));
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_write_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_lock(m_state));

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while locked or while readers are waiting

        int loop_count = 0;
        while (m_state != 0 || m_num_waiting_readers > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            m_waiting_writers.wait(l);
            --m_num_waiting_writers;
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        m_num_readers_to_wake = 0;

        //Writer priority: wait while locked

        int loop_count = 0;
        while (m_state != 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            m_waiting_writers.wait(l);
            --m_num_waiting_writers;
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        if (m_sp == read_write_scheduling_policy::alternating_single_read && m_num_waiting_writers == 0)
            m_num_readers_to_wake = (m_readers_next && m_num_readers_to_wake > 0) ? 1 : 0;

        //Alternating priority: wait while locked or while not writers' turn

        int loop_count = 0;
        while (m_state != 0 || m_num_readers_to_wake > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            m_waiting_writers.wait(l);
            --m_num_waiting_writers;
        }
    }

    //Obtain a write lock

    BOOST_ASSERT(valid_write_lockable(m_state));
    m_state = -1;

    //See note in read_write_mutex_impl<>::do_read_lock() as to why 
    //m_readers_next should be set here

    m_readers_next = true;

    BOOST_ASSERT(valid_write_lock(m_state));
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_read_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_lock(m_state));

    if (!l.locked())
        return false;

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
        fail = (m_state == -1 || m_num_readers_to_wake == 0);

        if (!fail)
        {
            BOOST_ASSERT(m_num_readers_to_wake > 0);
            --m_num_readers_to_wake;
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

        BOOST_ASSERT(valid_read_lock(m_state));
            //Should be read-locked
    }
    else
    {
        BOOST_ASSERT(valid_write_lock(m_state) || m_num_waiting_writers > 0);
            //Should be write-locked or
            //writer should be waiting
    }

    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_write_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_lock(m_state));

    if (!l.locked())
        return false;

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
        fail = (m_state != 0 || m_num_readers_to_wake > 0);
    }

    if (!fail)
    {
        //Obtain a write lock

        BOOST_ASSERT(valid_write_lockable(m_state));
        m_state = -1;

        //See note in read_write_mutex_impl<>::do_read_lock() as to why 
        //m_readers_next should be set here

        m_readers_next = true;

        BOOST_ASSERT(valid_write_lock(m_state));
            //Should be write-locked
    }
    else
    {
        BOOST_ASSERT(valid_read_write_lock(m_state) || m_num_readers_to_wake > 0);
            //Should be read-locked or write-locked, or
            //reader should be waking
    }

    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_read_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_ASSERT(valid_lock(m_state));

    if (!l.locked())
        return false;

    bool fail = false;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while write-locked

        int loop_count = 0;
        while (m_state == -1)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                --m_num_waiting_readers;
                fail = true; 
                break;
            }
            --m_num_waiting_readers;
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Writer priority: wait while write-locked or while writers are waiting

        int loop_count = 0;
        while (m_state == -1 || m_num_waiting_writers > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                --m_num_waiting_readers;
                fail = true; 
                break;
            }
            --m_num_waiting_readers;
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Alternating priority: wait while write-locked or while not readers' turn

        int loop_count = 0;
        while (m_state == -1 || m_num_readers_to_wake == 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_readers;
            if (!m_waiting_readers.timed_wait(l, xt))
            {
                --m_num_waiting_readers;
                fail = true; 
                break;
            }
            --m_num_waiting_readers;
        }

        if (!fail)
        {
            BOOST_ASSERT(m_num_readers_to_wake > 0);
            --m_num_readers_to_wake;
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

        BOOST_ASSERT(valid_read_lock(m_state));
            //Should be read-locked
    }
    else
    {
        if (m_num_readers_to_wake > 0)
        {
            //If there were readers scheduled to wake, 
            //decrement the number in case we were that reader.
            //If only one was scheduled to wake, the scheduling
            //algorithm will schedule another if one is available;
            //if more than one, one fewer reader will run before
            //the scheduling algorithm is called again. This last
            //case is not ideal, especially if a lot of waiting
            //readers timeout, but without knowing whether
            //we were actually one of the readers that was
            //scheduled to wake it's difficult to come up
            //with a better plan.
            --m_num_readers_to_wake;
        }

        if (m_state == 0)
        {
            //If there is no thread with a lock that will 
            //call do_scheduling_impl() when it unlocks, call it ourselves
            do_timeout_scheduling_impl();
        }
    }

    return !fail;
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_write_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_ASSERT(valid_lock(m_state));

    if (!l.locked())
        return false;

    bool fail = false;

    if (m_sp == read_write_scheduling_policy::reader_priority)
    {
        //Reader priority: wait while locked or while readers are waiting

        int loop_count = 0;
        while (m_state != 0 || m_num_waiting_readers > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                --m_num_waiting_writers;
                fail = true;
                break;
            }
            --m_num_waiting_writers;
        }
    }
    else if (m_sp == read_write_scheduling_policy::writer_priority)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        m_num_readers_to_wake = 0;

        //Writer priority: wait while locked

        int loop_count = 0;
        while (m_state != 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                --m_num_waiting_writers;
                fail = true;
                break;
            }
            --m_num_waiting_writers;
        }
    }
    else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_single_read || m_sp == read_write_scheduling_policy::alternating_many_reads)
    {
        //Shut down extra readers that were scheduled only because of no waiting writers

        if (m_sp == read_write_scheduling_policy::alternating_single_read && m_num_waiting_writers == 0)
            m_num_readers_to_wake = (m_readers_next && m_num_readers_to_wake > 0) ? 1 : 0;

        //Alternating priority: wait while locked or while not writers' turn

        int loop_count = 0;
        while (m_state != 0 || m_num_readers_to_wake > 0)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            ++m_num_waiting_writers;
            if (!m_waiting_writers.timed_wait(l, xt))
            {
                --m_num_waiting_writers;
                fail = true;
                break;
            }
            --m_num_waiting_writers;
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

        BOOST_ASSERT(valid_write_lock(m_state));
            //Should be write-locked
    }
    else
    {
        if (m_state == 0)
        {
            //If there is no thread with a lock that will 
            //call do_scheduling_impl() when it unlocks, call it ourselves
            do_timeout_scheduling_impl();
        }
    }

    return !fail;
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_read_unlock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_read_lock(m_state));

    if (m_state > 0)
        --m_state;
    else //not read-locked
        throw lock_error();

    if (m_state == 0)
        do_unlock_scheduling_impl();

    BOOST_ASSERT(valid_lock(m_state));
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_write_unlock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_write_lock(m_state));

    if (m_state == -1)
        m_state = 0;
    else BOOST_ASSERT_ELSE(m_state >= 0)
        throw lock_error();      // Trying to release a reader-locked or unlocked mutex???

    do_unlock_scheduling_impl();

    BOOST_ASSERT(valid_lock(m_state));
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_demote_to_read_lock_impl()
{
    BOOST_ASSERT(valid_write_lock(m_state));

    if (m_state == -1) 
    {
        //Convert from write lock to read lock
        m_state = 1;

        //If the conditions are right, release other readers

        do_demote_scheduling_impl();

        //Lock demoted
        BOOST_ASSERT(valid_read_lock(m_state));
        return true;
    }
    else BOOST_ASSERT_ELSE(m_state >= 0)
    {
        //Lock is read-locked or unlocked can't be demoted
        throw lock_error();
        return false;
    }
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_demote_to_read_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_write_lock(m_state));

    do_demote_to_read_lock_impl();
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_demote_to_read_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_write_lock(m_state));

    if (!l.locked())
        return false;
    else //(l.locked())
        return do_demote_to_read_lock_impl();
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_demote_to_read_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_ASSERT(valid_write_lock(m_state));

    if (!l.locked())
        return false;
    else //(l.locked())
        return do_demote_to_read_lock_impl();
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_promote_to_write_lock()
{
    typename Mutex::scoped_lock l(m_prot);
    BOOST_ASSERT(valid_read_lock(m_state));

    if (m_state == 1)
    {
        //Convert from read lock to write lock
        m_state = -1;

        //Lock promoted
        BOOST_ASSERT(valid_write_lock(m_state));
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
        ++m_num_waiting_writers;
        m_state_waiting_promotion = true;

        int loop_count = 0;
        while (m_state > 1)
        {
            BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
            m_waiting_promotion.wait(l);
        }

        m_state_waiting_promotion = false;
        --m_num_waiting_writers;
        
        BOOST_ASSERT(m_num_waiting_writers >= 0);
        BOOST_ASSERT(m_state == 1);

        //Convert from read lock to write lock
        m_state = -1;
        
        //Lock promoted
        BOOST_ASSERT(valid_write_lock(m_state));
    }
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_try_promote_to_write_lock()
{
    typename Mutex::scoped_try_lock l(m_prot);
    BOOST_ASSERT(valid_read_lock(m_state));

    if (!l.locked())
        return false;
    else
    {
        if (m_state == 1)
        {
            //Convert from read lock to write lock
            m_state = -1;

            //Lock promoted
            BOOST_ASSERT(valid_write_lock(m_state));
            return true;
        }
        else if (m_state <= 0)
        {
            //Lock is write-locked or unlocked can't be promoted
            throw lock_error();
        }
        else if (m_state_waiting_promotion)
        {
            //Someone else is already trying to promote. Avoid deadlock by returning false.
            return false;
        }
        else BOOST_ASSERT_ELSE(m_state > 1 && !m_state_waiting_promotion)
        {
            //There are other readers, so we can't promote
            return false;
        }
    }
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::do_timed_promote_to_write_lock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot, xt);
    BOOST_ASSERT(valid_read_lock(m_state));

    if (!l.locked())
        return false;
    else
    {
        if (m_state == 1)
        {
            //Convert from read lock to write lock
            m_state = -1;
            
            //Lock promoted
            BOOST_ASSERT(valid_write_lock(m_state));
            return true;
        }
        else if (m_state <= 0)
        {
            //Lock is not read-locked and can't be promoted
            throw lock_error();
        }
        else if (m_state_waiting_promotion)
        {
            //Someone else is already trying to promote. Avoid deadlock by returning false.
            return false;
        }
        else BOOST_ASSERT_ELSE(m_state > 1 && !m_state_waiting_promotion)
        {   
            ++m_num_waiting_writers;
            m_state_waiting_promotion = true;

            int loop_count = 0;
            while (m_state > 1)
            {
                BOOST_ASSERT(++loop_count == 1); //Check for invalid loop conditions (but will also detect spurious wakeups)
                if (!m_waiting_promotion.timed_wait(l, xt))
                {
                    m_state_waiting_promotion = false;
                    --m_num_waiting_writers;
                    return false;
                }
            }

            m_state_waiting_promotion = false;
            --m_num_waiting_writers;
            
            BOOST_ASSERT(m_num_waiting_writers >= 0);
            BOOST_ASSERT(m_state == 1);

            //Convert from read lock to write lock
            m_state = -1;
            
            //Lock promoted
            BOOST_ASSERT(valid_write_lock(m_state));
            return true;
        }
    }
}

template<typename Mutex>
bool read_write_mutex_impl<Mutex>::locked()
{
    int state = m_state;
    BOOST_ASSERT(valid_lock(state));

    return state != 0;
}

template<typename Mutex>
read_write_lock_state::read_write_lock_state_enum read_write_mutex_impl<Mutex>::state()
{
    int state = m_state;
    BOOST_ASSERT(valid_lock(state));

    if (state > 0)
    {
        BOOST_ASSERT(valid_read_lock(state));
        return read_write_lock_state::read_locked;
    }
    else if (state == -1)
    {
        BOOST_ASSERT(valid_write_lock(state));
        return read_write_lock_state::write_locked;
    }
    else BOOST_ASSERT_ELSE(state == 0)
        return read_write_lock_state::unlocked;
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_unlock_scheduling_impl()
{
    BOOST_ASSERT(m_state == 0);
    do_scheduling_impl();
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_timeout_scheduling_impl()
{
    BOOST_ASSERT(m_state == 0);
    do_scheduling_impl();
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_demote_scheduling_impl()
{
    BOOST_ASSERT(m_state == 1);
    do_scheduling_impl();
}

template<typename Mutex>
void read_write_mutex_impl<Mutex>::do_scheduling_impl()
{
    bool demotion = m_state > 0; //Releasing readers after lock demotion?
    
    BOOST_ASSERT(valid_read_lockable(m_state));

    if (m_num_waiting_writers > 0 && m_num_waiting_readers > 0)
    {
        //Both readers and writers waiting: use scheduling policy
        if (m_sp == read_write_scheduling_policy::reader_priority)
        {
            m_num_readers_to_wake = m_num_waiting_readers;
            m_waiting_readers.notify_all();
        }
        else if (m_sp == read_write_scheduling_policy::writer_priority)
        {
            if (!demotion)
            {
                if (m_state_waiting_promotion)
                    m_waiting_promotion.notify_one();
                else
                    m_waiting_writers.notify_one();
            }
        }
        else if (m_sp == read_write_scheduling_policy::alternating_single_read)
        {
            if (m_num_readers_to_wake > 0)
            {
                //Let the already woken threads work
            }
            else  if (m_readers_next)
            {
                m_num_readers_to_wake = 1;
                m_waiting_readers.notify_one();
            }
            else
            {
                if (!demotion)
                {
                    if (m_state_waiting_promotion)
                        m_waiting_promotion.notify_one();
                    else
                        m_waiting_writers.notify_one();
                }
            }
        }
        else BOOST_ASSERT_ELSE(m_sp == read_write_scheduling_policy::alternating_many_reads)
        {
            if (m_num_readers_to_wake > 0)
            {
                //Let the already woken threads work
            }
            else if (m_readers_next)
            {
                m_num_readers_to_wake = m_num_waiting_readers;
                m_waiting_readers.notify_all();
            }
            else
            {
                if (!demotion)
                {
                    if (m_state_waiting_promotion)
                        m_waiting_promotion.notify_one();
                    else
                        m_waiting_writers.notify_one();
                }
            }
        }
    }
    else if (m_num_waiting_writers > 0)
    {
        if (!demotion)
        {
            //Only writers waiting--scheduling policy doesn't matter
            if (m_state_waiting_promotion)
                m_waiting_promotion.notify_one();
            else
                m_waiting_writers.notify_one();
        }
    }
    else if (m_num_waiting_readers > 0)
    {
        //Only readers waiting--scheduling policy doesn't matter
        m_num_readers_to_wake = m_num_waiting_readers;
        m_waiting_readers.notify_all();
    }
}

    }   // namespace thread
    }   // namespace detail


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
