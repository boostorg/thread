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

#include <boost/thread/rw_mutex.hpp>
#include <cassert>

namespace boost {
namespace detail {
namespace thread {

template<typename Mutex>
void rw_mutex_impl<Mutex>::do_rdlock()
{
    // Lock our exclusive access.  This protects internal state
    typename Mutex::scoped_lock l(m_prot);

    // Wait until no exclusive lock is held.
    //
    // Note:  Scheduling priorities are enforced in the unlock()
    //   call.  unlock will wake the proper thread.
    while(m_state < 0)
    {
        m_num_waiting_readers++;
        m_waiting_readers.wait(l);
        m_num_waiting_readers--;
    }

    // Increase the reader count
    m_state++;
}

template<typename Mutex>
void rw_mutex_impl<Mutex>::do_wrlock()
{
    // Lock our exclusive access.  This protects internal state
    typename Mutex::scoped_lock l(m_prot);

    // Wait until no exclusive lock is held.
    //
    // Note:  Scheduling priorities are enforced in the unlock()
    //   call.  unlock will wake the proper thread.
    while(m_state != 0)
    {
        m_num_waiting_writers++;
        m_waiting_writers.wait(l);
        m_num_waiting_writers--;
    }
    m_state = -1;
}

template<typename Mutex>
bool rw_mutex_impl<Mutex>::do_try_rdlock()
{
    bool ret;
    // Lock our exclusive access.  This protects internal state

    typename Mutex::scoped_lock l(m_prot);
    if(!l.locked())
        return false;

    if(m_state == -1)
    {
        // We are already locked exclusively.  A try_rdlock always returns
        //   immediately in this case
        ret =  false;
    }
    else if(m_num_waiting_writers > 0)
    {
        // There are also waiting writers.  Use scheduling policy.
        if(m_sp == sp_reader_priority)
        {
            m_state++;
            ret = true;
        }
        else if(m_sp == sp_writer_priority)
        {
            // A writer is waiting - don't grant this try lock, and
            //   return immediately (don't increase waiting_readers count)
            ret = false;
        }
        else
        {
            // For alternating scheduling priority,
            // I don't think that try_ locks should step in front of others
            //   who have already indicated that they are waiting.
            // It seems that this could "game" the system and defeat
            //   the alternating mechanism.
            ret = false;
        }
    }
    else
    {
        // No waiting writers.  Grant (additonal) read lock regardless of
        //   scheduling policy.
        m_state++;
        ret = true;
    }

    return ret;
}

template<typename Mutex>
bool rw_mutex_impl<Mutex>::do_try_wrlock()
{
    bool ret;

    typename Mutex::scoped_lock l(m_prot);
    if(!l.locked())
        return false;

    if(m_state != 0)
    {
        // We are already busy and locked.
        // Scheduling priority doesn't matter here.
        ret = false;
    }
    else
    {
        m_state = -1;
        ret = true;
    }

    return ret;
}

template<typename Mutex>
bool rw_mutex_impl<Mutex>::do_timed_rdlock(const boost::xtime &xt)
{
    // Lock our exclusive access.  This protects internal state
    typename Mutex::scoped_timed_lock l(m_prot,xt);
    if(!l.locked())
        return false;


    // Wait until no exclusive lock is held.
    //
    // Note:  Scheduling priorities are enforced in the unlock()
    //   call.  unlock will wake the proper thread.
    while(m_state < 0)
    {
        m_num_waiting_readers++;
        if(!m_waiting_readers.timed_wait(l,xt))
        {
            m_num_waiting_readers--;
            return false;
        }
        m_num_waiting_readers--;
    }

    // Increase the reader count
    m_state++;
    return true;
}

template<typename Mutex>
bool rw_mutex_impl<Mutex>::do_timed_wrlock(const boost::xtime &xt)
{
    typename Mutex::scoped_timed_lock l(m_prot,xt);

    if(!l.locked())
        return false;

    // Wait until no exclusive lock is held.
    //
    // Note:  Scheduling priorities are enforced in the unlock()
    //   call.  unlock will wake the proper thread.
    while(m_state != 0)
    {
        m_num_waiting_writers++;
        if(!m_waiting_writers.timed_wait(l,xt))
        {
            m_num_waiting_writers--;
            return false;
        }
        m_num_waiting_writers--;
    }
    m_state = -1;
    return true;
}

template<typename Mutex>
void rw_mutex_impl<Mutex>::do_rdunlock()
{
    // Protect internal state.
    typename Mutex::scoped_lock l(m_prot);
    if(m_state > 0)        // Release a reader.
        m_state--;
    else
        throw lock_error();     // Trying to release a writer???

    // If we have someone waiting to be promoted....
    if(m_num_waiting_promotion == 1 && m_state == 1)
    {
        m_waiting_promotion.notify_one();
    }
    else if(m_state == 0)
    {
        do_wakeups();
    }
}

template<typename Mutex>
void rw_mutex_impl<Mutex>::do_wakeups()
{
    if( m_num_waiting_writers > 0 &&
        m_num_waiting_readers > 0)
    {
        // We have both types waiting, and -either- could proceed.
        //    Choose which to release based on scheduling policy.
        if(m_sp == sp_reader_priority)
        {
            m_waiting_readers.notify_all();
        }
        else if(m_sp == sp_writer_priority)
        {
            m_waiting_writers.notify_one();
        }
        else // one of the alternating mechanisms
        {
            if(m_readers_next == 1)
            {
                m_readers_next = 0;
                if(m_sp == sp_alternating_many_reads)
                {
                    m_waiting_readers.notify_all();
                }
                else
                {
                    // sp_alternating_single_reads
                    m_waiting_readers.notify_one();
                }
            }
            else
            {
                m_waiting_writers.notify_one();
                m_readers_next = 1;
            }
        }
    }
    else if(m_num_waiting_writers > 0)
    {
        // Only writers - scheduling doesn't matter
        m_waiting_writers.notify_one();
    }
    else if(m_num_waiting_readers > 0)
    {
        // Only readers - scheduling doesn't matter
        m_waiting_readers.notify_all();
    }
}

template<typename Mutex>
void rw_mutex_impl<Mutex>::do_wrunlock()
{
    // Protect internal state.
    typename Mutex::scoped_lock l(m_prot);

    if(m_state == -1)
        m_state = 0;
    else
        throw lock_error();

    // After a writer is unlocked, we are always back in the unlocked state.
    //
    do_wakeups();
}

}   // namespace thread
}   // namespace detail

void rw_mutex::do_rdlock()
{
    m_impl.do_rdlock();
}

void rw_mutex::do_wrlock()
{
    m_impl.do_wrlock();
}

void rw_mutex::do_rdunlock()
{
    m_impl.do_rdunlock();
}

void rw_mutex::do_wrunlock()
{
    m_impl.do_wrunlock();
}

void try_rw_mutex::do_rdlock()
{
    m_impl.do_rdlock();
}

void try_rw_mutex::do_wrlock()
{
    m_impl.do_wrlock();

}

void try_rw_mutex::do_wrunlock()
{
    m_impl.do_wrunlock();
}

void try_rw_mutex::do_rdunlock()
{
    m_impl.do_rdunlock();
}

bool try_rw_mutex::do_try_rdlock()
{
    return m_impl.do_try_rdlock();
}

bool try_rw_mutex::do_try_wrlock()
{
    return m_impl.do_try_wrlock();
}

void timed_rw_mutex::do_rdlock()
{
    m_impl.do_rdlock();
}

void timed_rw_mutex::do_wrlock()
{
    m_impl.do_wrlock();

}

void timed_rw_mutex::do_rdunlock()
{
    m_impl.do_rdunlock();
}

void timed_rw_mutex::do_wrunlock()
{
    m_impl.do_wrunlock();
}

bool timed_rw_mutex::do_try_rdlock()
{
    return m_impl.do_try_rdlock();
}

bool timed_rw_mutex::do_try_wrlock()
{
    return m_impl.do_try_wrlock();
}

bool timed_rw_mutex::do_timed_rdlock(const xtime &xt)
{
    return m_impl.do_timed_rdlock(xt);
}

bool timed_rw_mutex::do_timed_wrlock(const xtime &xt)
{
    return m_impl.do_timed_wrlock(xt);
}

} // namespace boost
