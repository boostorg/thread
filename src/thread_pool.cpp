// Copyright (C) 2002 David Moore
//
// Based on Boost.Threads
// Copyright (C) 2001
// William E. Kempf
//
// Derived loosely from work queue manager in "Programming POSIX Threads"
//   by David Butenhof.
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.


#include <boost/thread/thread_pool.hpp>
#include <boost/thread/detail/thread_pool_impl.hpp>
#include <stdexcept>
#include <boost/thread/xtime.hpp>


namespace
{

    // For launching pool threads w/ arguments
class thread_adapter
{
public:
    thread_adapter(void (*func)(void*), void* param) 
        : _func(func), _param(param){ }
    void operator()() const { _func(_param); }
private:
        void (*_func)(void*);
        void* _param;
};


}




namespace boost
{

thread_pool::
thread_pool(int max_threads, int min_threads,int timeout_secs) : 
    m_pimpl(new detail::thread::thread_pool_impl(max_threads,min_threads,timeout_secs))
{
} 


thread_pool::
~thread_pool()
{
    if(m_pimpl != NULL)
        delete m_pimpl;
}



void 
thread_pool::
add(const boost::function0<void> &job)
{
    if(m_pimpl)
        m_pimpl->add(job);
    else
        throw std::runtime_error("Can't add after detach");
}


void 
thread_pool::
join()
{
    if(m_pimpl)
        m_pimpl->join();
    else
        throw std::runtime_error("Can't join after detach");
}

void 
thread_pool::
cancel()
{
    if(m_pimpl)
        m_pimpl->cancel();
    else
        throw std::runtime_error("Can't cancel after detach");
}


void 
thread_pool::
detach()
{
    if(m_pimpl)
    {
        // Tell our implementation it is running detached.
        m_pimpl->detach();
        m_pimpl = NULL;
    }
    else
        throw std::runtime_error("Already detached");
}



namespace detail { namespace thread {


thread_pool_impl::
thread_pool_impl(int max_threads, int min_threads,int timeout_secs) : 
    m_state(RUNNING),
    m_max_threads(max_threads),
    m_min_threads(min_threads),
    m_thread_count(0),
    m_idle_count(0),
    m_timeout_secs(timeout_secs)
{
    // Immediately launch some worker threads.
    //
    // Not an exception safe implementation, yet.
    while(min_threads-- > 0)
    {
        m_workers.create_thread(thread_adapter(_worker_harness,this));
        m_thread_count++;
    }
}


thread_pool_impl::
~thread_pool_impl()
{
    // Join in the destructor, unless they have already
    //    joined or detached.

    mutex::scoped_lock l(m_prot);
    if(m_state == RUNNING)
    {
        l.unlock();
        join();
    }
}


void
thread_pool_impl::
join()
{
    mutex::scoped_lock l(m_prot);
 
    if(m_state != RUNNING)
    {
        throw std::runtime_error("Can't join pool unless RUNNING");
    }

    if(m_thread_count > 0)
    {
        m_state = JOINING;

        // if any threads are idling, wake them. 
        if (m_idle_count > 0) 
        {
            m_more_work.notify_all();
        }

        // Track the shutdown progress of the threads.
        while(m_thread_count > 0)
        {
            m_done.wait(l);
        }
    }

    m_workers.join_all();
    m_state = JOINED;
}



// This is a "weak" form of cancel which empties out the job queue and takes
//   the thread count down to zero.
//
// Upon receiving more work, the thread count would grow back up to min_threads.
//
// Cancel will be much stronger once full thread cancellation is in place!

void 
thread_pool_impl::
cancel()
{
    mutex::scoped_lock l(m_prot);
 
    if(m_state != RUNNING)
    {
        throw std::runtime_error("Can't cancel pool unless RUNNING");
    }

    if(m_thread_count > 0)
    {
        m_state = CANCELLING;
        // Cancelling kills any unexecuted jobs.
        while(!m_jobs.empty())
            m_jobs.pop();


        /* If we had cancel, this would be something like....

        m_workers.cancel_all();
        while(m_cancel_count > 0)
        {
            m_all_cancelled.wait(l);
        }

        */
    }
    m_state = RUNNING;          // Go back to accepting work.
}


void
thread_pool_impl::
detach()
{
    mutex::scoped_lock l(m_prot);
    if(m_state == RUNNING)
    {
        m_min_threads = 0;
        m_state = DETACHED;
    }
    else
    {
        // detach during/after a join has no effect - the join will
        //   complete.
    }
}


void
thread_pool_impl::
add(const boost::function0<void> &job)
{
    mutex::scoped_lock l(m_prot);
    
    // Note - can never reach this point if m_state == CANCELLED
    //  because the m_prot is held during the entire cancel operation.

    if(m_state != RUNNING)
    {
        throw std::runtime_error("Can't add job to pool unless RUNNING");
    }


    m_jobs.push(job);
    if(m_idle_count > 0)
    {
        m_more_work.notify_one();
    }
    else if(m_thread_count < m_max_threads)
    {
        // No idle threads, and we're below our limit.  Spawn a new
        //  worker.
        
        // What we really need is thread::detach(), or "create suspended"
        m_workers.create_thread(thread_adapter(_worker_harness,this));
        m_thread_count++;
    }
}



void 
thread_pool_impl::
_worker_harness(void *arg)
{
    thread_pool_impl *pthis = reinterpret_cast<thread_pool_impl *>(arg);
    boost::thread me;

    xtime   timeout;
    int     timedout;

    mutex::scoped_lock l(pthis->m_prot);

    while (1) {
        timedout = 0;

        xtime_get(&timeout,boost::TIME_UTC);
        timeout.sec += pthis->m_timeout_secs;
        

        while(pthis->m_jobs.empty() && (pthis->m_state == RUNNING))
        {
            pthis->m_idle_count++;
            bool status = pthis->m_more_work.timed_wait(l,timeout);
            pthis->m_idle_count--;
            if(!status)
            {
                timedout = 1;
                break;
            }
        }

        if(!pthis->m_jobs.empty() && pthis->m_state != CANCELLING)
        {
            boost::function0<void> jobfunc = pthis->m_jobs.front();
            pthis->m_jobs.pop();
            l.unlock();
            jobfunc();
            l.lock();
        }
        else if(pthis->m_jobs.empty() && pthis->m_state == JOINING)
        {
            pthis->m_thread_count--;
     
            // If we are the last worker exiting, let everyone know about it!
            if(pthis->m_thread_count == 0)
            {
                pthis->m_done.notify_all();
            }
            break;
        }
        else if(pthis->m_jobs.empty() && pthis->m_state == DETACHED)
        {
            pthis->m_thread_count--;
            // If we are the last worker exiting, let everyone know about it!
            if(pthis->m_thread_count == 0)
            {
                l.unlock();
                delete pthis;
            }
            break;
        }



        /*
         * If there's no more work, and we wait for as long as
         * we're allowed, then terminate this server thread.
         */
        if (pthis->m_jobs.empty() && timedout ) {
            if(pthis->m_thread_count > pthis->m_min_threads)
            {
                pthis->m_thread_count--;

                if(pthis->m_state == DETACHED && 
                    pthis->m_thread_count == 0)
                {
                    l.unlock();
                    delete pthis;
                    break;
                }

                // We aren't in a JOINING or CANCELLING state, so trim
                //   down our resource usage and clean ourselves up.
                pthis->m_workers.delete_thread_equal_to(&me);
                break;
            }
        }
    }

    return;
}

thread_grp::
thread_grp()
{
}

thread_grp::
~thread_grp()
{
    for (std::list<boost::thread*>::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
        delete (*it);
}

boost::thread* 
thread_grp::
create_thread(const function0<void>& threadfunc)
{
    std::auto_ptr<boost::thread> thrd(new boost::thread(threadfunc));
    m_threads.push_back(thrd.get());
    return thrd.release();
}


// Delete the thread in the group with identity equal to thrd
void 
thread_grp::
delete_thread_equal_to(boost::thread *thrd)
{
    std::list<boost::thread*>::iterator it = m_threads.begin();
    for(;it != m_threads.end(); ++it)
    {
        if(**it == *thrd)
            break;
    }
    assert(it != m_threads.end());
    if (it != m_threads.end())
    {
        boost::thread *pthread = *it;
        m_threads.erase(it);
        delete pthread;
    }
}

void thread_grp::join_all()
{
    for (std::list<boost::thread*>::iterator it = 
        m_threads.begin(); it != m_threads.end(); ++it)
        (*it)->join();
}

}   // namespace thread
}   // namespace detail



}	// namespace boost
