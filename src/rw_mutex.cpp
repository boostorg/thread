// rw_mutex.cpp
//
// Implementaion for Reader/Writer lock
//
// Notes:
//
// This implementation is based on ACE's rw_lock_t implementation, with the added
//   functionality of supporting different Scheduling Policies.
//
// The underlying implementation is shared by rw_mutex, try_rw_mutex, and 
//    timed_rw_mutex.
//
// The basic implementation strategy involves a mutex, m_prot which is locked during
//   ANY rw_mutex operation, locking or unlocking.  m_prot protects the invariants
//   of the implementation.
//
// The variable m_state takes the following values:
//   -1 - Exclusive locked
//    0 - Unlocked
//   1 -> INT_MAX, shared locked, m_state == # of shared locks.
//
// Should a thread need to block for a shared or exclusive lock, two 
//   member condition variables, m_waiting_readers and m_waiting_writers
//   are available for waiting.  m_prot is used as the controlling mutex
//   when waiting in these cases.
//
// The number of waiting readers and writers are tracked via member variables
//   m_num_waiting_readers and m_num_waiting_writers.
//
//
// This particular implementation cannot prevent self-deadlock w/o adding some means
//  of identifying the thread(s) holding locks.  

//  A recursive_try_mutex used "under the hood" could be used to detect and prevent 
//  exclusive->exclusive self-deadlock since only the same thread would be able to
//  obtain a second lock on this recursive mutex....
/*
//  for example, if rw_mutex_impl has an additional member:
//
//  struct rw_mutex_impl {
//      // ...
//      recursive_try_mutex m_self_detect;
//  }
//

template<typename Mutex>
void
rw_mutex_impl<Mutex>::
do_wrlock()
{
    // Lock our exclusive access.  This protects internal state
    Mutex::scoped_lock l(m_prot);

    if(m_state == -1)
    {
        recursive_try_mutex sl(m_self_detect);
        if(sl.locked())
        { 
            // It is us that already held the lock!
            
            // Do something to hold the m_self_detect lock
            // and bail out 
        }
        else
        {
            // It is someone else.  fall back to normal waiting.
        }
    }

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


// Unfortunately, the above doesn't work to detect shared->exclusive deadlock where
//   a shared lock holder tries for an exclusive lock.


*/


#include <boost/thread/rw_mutex.hpp>
#include <cassert>

namespace boost {
    namespace detail { namespace thread {

   



template<typename Mutex>
void
rw_mutex_impl<Mutex>::
do_rdlock()
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
void
rw_mutex_impl<Mutex>::
do_wrlock()
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
bool
rw_mutex_impl<Mutex>::
do_try_rdlock()
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


/*
 *
 * try_promote_rdlock - not yet in production....
 *
 *
 *

template<typename Mutex>
bool
rw_mutex_impl<Mutex>::
do_try_promote_rdlock()
{
    RWMutexImpl::scoped_lock l(m_prot);

    if(m_state == -1)
    {
        // promoting a write-locked to a read lock is a serious error.
        throw lock_error();
    }
    else if(m_num_waiting_promotion > 0)
    {
        // Someone else is already trying to upgrade.  Avoid deadlock by
        //   returning false.
        return false;
    }
    else
    {
        while(m_state > 1)     // While there are other readers
        {
            m_num_waiting_writers++;
            m_num_waiting_promotion = 1;
            m_waiting_promotion.wait(l);
            m_num_waiting_promotion = 0;
            m_num_waiting_writers--;
        }
        // We got the exclusive lock!
        m_state == -1;
    }
}
*/


template<typename Mutex>
bool
rw_mutex_impl<Mutex>::
do_try_wrlock()
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
bool
rw_mutex_impl<Mutex>::
do_timed_rdlock(const boost::xtime &xt)
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
bool
rw_mutex_impl<Mutex>::
do_timed_wrlock(const boost::xtime &xt)
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
void
rw_mutex_impl<Mutex>::
do_rdunlock()
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
void
rw_mutex_impl<Mutex>::
do_wakeups()
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
void
rw_mutex_impl<Mutex>::
do_wrunlock()
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




    
void 
rw_mutex::
do_rdlock()
{
    m_impl.do_rdlock();
}


void 
rw_mutex::
do_wrlock()
{
    m_impl.do_wrlock();
}


void 
rw_mutex::
do_rdunlock()
{
    m_impl.do_rdunlock();
}

void 
rw_mutex::
do_wrunlock()
{
    m_impl.do_wrunlock();
}

void 
try_rw_mutex::
do_rdlock()
{
    m_impl.do_rdlock();
}


  

void 
try_rw_mutex::
do_wrlock()
{
    m_impl.do_wrlock();

}


void 
try_rw_mutex::
do_wrunlock()
{
    m_impl.do_wrunlock();
}

void 
try_rw_mutex::
do_rdunlock()
{
    m_impl.do_rdunlock();
}

bool 
try_rw_mutex::
do_try_rdlock()
{
    return m_impl.do_try_rdlock();
}

bool 
try_rw_mutex::
do_try_wrlock()
{
    return m_impl.do_try_wrlock();
}







void 
timed_rw_mutex::
do_rdlock()
{
    m_impl.do_rdlock();
}


  

void 
timed_rw_mutex::
do_wrlock()
{
    m_impl.do_wrlock();

}



void 
timed_rw_mutex::
do_rdunlock()
{
    m_impl.do_rdunlock();
}

void 
timed_rw_mutex::
do_wrunlock()
{
    m_impl.do_wrunlock();
}



bool 
timed_rw_mutex::
do_try_rdlock()
{
    return m_impl.do_try_rdlock();
}


bool 
timed_rw_mutex::
do_try_wrlock()
{
    return m_impl.do_try_wrlock();
}



bool 
timed_rw_mutex::
do_timed_rdlock(const xtime &xt)
{
    return m_impl.do_timed_rdlock(xt);
}


bool 
timed_rw_mutex::
do_timed_wrlock(const xtime &xt)
{
    return m_impl.do_timed_wrlock(xt);
}





} // namespace boost