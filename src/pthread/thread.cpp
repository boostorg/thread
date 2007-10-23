// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/locks.hpp>
#include <cassert>

#include "timeconv.inl"

namespace {

class thread_param
{
public:
    thread_param(const boost::function0<void>& threadfunc)
        : m_threadfunc(threadfunc), m_started(false)
    {
    }
    void wait()
    {
        boost::mutex::scoped_lock scoped_lock(m_mutex);
        while (!m_started)
            m_condition.wait(scoped_lock);
    }
    void started()
    {
        boost::mutex::scoped_lock scoped_lock(m_mutex);
        m_started = true;
        m_condition.notify_one();
    }

    boost::mutex m_mutex;
    boost::condition m_condition;
    const boost::function0<void>& m_threadfunc;
    bool m_started;
};

} // unnamed namespace

extern "C" {
        static void* thread_proxy(void* param)
    {
        thread_param* p = static_cast<thread_param*>(param);
        boost::function0<void> threadfunc = p->m_threadfunc;
        p->started();
        threadfunc();
        return 0;
    }

}

namespace boost {

thread::thread():
    m_joinable(false)
{}

thread::thread(const function0<void>& threadfunc)
    : m_joinable(true)
{
    thread_param param(threadfunc);
    int res = 0;
    pthread_t m_thread;
    res = pthread_create(&m_thread, 0, &thread_proxy, &param);
    if (res != 0)
        throw thread_resource_error();
    m_id=m_thread;
    param.wait();
}

thread::~thread()
{
    if (m_joinable)
    {
        pthread_detach(*m_id.id);
    }
}

bool thread::operator==(const thread& other) const
{
    return m_id==other.m_id;
}

bool thread::operator!=(const thread& other) const
{
    return !operator==(other);
}

void thread::join()
{
    assert(m_joinable); //See race condition comment below
    int res = 0;
    res = pthread_join(*m_id.id, 0);
    assert(res == 0);
    // This isn't a race condition since any race that could occur would
    // have us in undefined behavior territory any way.
    m_joinable = false;
}

void thread::sleep(const xtime& xt)
{
    for (int foo=0; foo < 5; ++foo)
    {
#   if defined(BOOST_HAS_PTHREAD_DELAY_NP)
        timespec ts;
        to_timespec_duration(xt, ts);
        int res = 0;
        res = pthread_delay_np(&ts);
        assert(res == 0);
#   elif defined(BOOST_HAS_NANOSLEEP)
        timespec ts;
        to_timespec_duration(xt, ts);

        //  nanosleep takes a timespec that is an offset, not
        //  an absolute time.
        nanosleep(&ts, 0);
#   else
        mutex mx;
        mutex::scoped_lock lock(mx);
        condition cond;
        cond.timed_wait(lock, xt);
#   endif
        xtime cur;
        xtime_get(&cur, TIME_UTC);
        if (xtime_cmp(xt, cur) <= 0)
            return;
    }
}

void thread::yield()
{
#   if defined(BOOST_HAS_SCHED_YIELD)
    int res = 0;
    res = sched_yield();
    assert(res == 0);
#   elif defined(BOOST_HAS_PTHREAD_YIELD)
    int res = 0;
    res = pthread_yield();
    assert(res == 0);
#   else
    xtime xt;
    xtime_get(&xt, TIME_UTC);
    sleep(xt);
#   endif
}

thread_group::thread_group()
{
}

thread_group::~thread_group()
{
    // We shouldn't have to scoped_lock here, since referencing this object
    // from another thread while we're deleting it in the current thread is
    // going to lead to undefined behavior any way.
    for (std::list<thread*>::iterator it = m_threads.begin();
         it != m_threads.end(); ++it)
    {
        delete (*it);
    }
}

thread* thread_group::create_thread(const function0<void>& threadfunc)
{
    // No scoped_lock required here since the only "shared data" that's
    // modified here occurs inside add_thread which does scoped_lock.
    std::auto_ptr<thread> thrd(new thread(threadfunc));
    add_thread(thrd.get());
    return thrd.release();
}

void thread_group::add_thread(thread* thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to add a thread object multiple
    // times. Should we consider this an error and either throw or return an
    // error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(),
        m_threads.end(), thrd);
    assert(it == m_threads.end());
    if (it == m_threads.end())
        m_threads.push_back(thrd);
}

void thread_group::remove_thread(thread* thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to remove a thread object that's
    // not in the group. Should we consider this an error and either throw or
    // return an error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(),
        m_threads.end(), thrd);
    assert(it != m_threads.end());
    if (it != m_threads.end())
        m_threads.erase(it);
}

void thread_group::join_all()
{
    mutex::scoped_lock scoped_lock(m_mutex);
    for (std::list<thread*>::iterator it = m_threads.begin();
         it != m_threads.end(); ++it)
    {
        (*it)->join();
    }
}

int thread_group::size() const
{
        return m_threads.size();
}

} // namespace boost
