// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.  
// It is provided "as is" without express or implied warranty.

#include <boost/thread/thread.hpp>
#include <boost/thread/semaphore.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <process.h>
#endif

#include "timeconv.inl"

namespace {

class thread_param
{
public:
    thread_param(const boost::function0<void>& threadfunc) : m_threadfunc(threadfunc), m_started(false) { }
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
#if defined(BOOST_HAS_WINTHREADS)
unsigned __stdcall thread_proxy(void* param)
#elif defined(BOOST_HAS_PTHREADS)
static void* thread_proxy(void* param)
#endif
{
    try
    {
        thread_param* p = static_cast<thread_param*>(param);
        boost::function0<void> threadfunc = p->m_threadfunc;
        p->started();
        threadfunc();
    }
    catch (...)
    {
    }
    return 0;
}

}

namespace boost {

thread::thread()
    : m_joinable(false)
{
#if defined(BOOST_HAS_WINTHREADS)
    m_thread = reinterpret_cast<unsigned long>(GetCurrentThread());
    m_id = GetCurrentThreadId();
#elif defined(BOOST_HAS_PTHREADS)
    m_thread = pthread_self();
#endif
}

thread::thread(const function0<void>& threadfunc)
    : m_joinable(true)
{
    thread_param param(threadfunc);
#if defined(BOOST_HAS_WINTHREADS)
    m_thread = _beginthreadex(0, 0, &thread_proxy, &param, 0, &m_id);
    if (!m_thread)
        throw thread_resource_error();
#elif defined(BOOST_HAS_PTHREADS)
    int res = 0;
    res = pthread_create(&m_thread, 0, &thread_proxy, &param);
    if (res != 0)
        throw thread_resource_error();
#endif
    param.wait();
}

thread::~thread()
{
    if (m_joinable)
    {
#if defined(BOOST_HAS_WINTHREADS)
        int res = 0;
        res = CloseHandle(reinterpret_cast<HANDLE>(m_thread));
        assert(res);
#elif defined(BOOST_HAS_PTHREADS)
        pthread_detach(m_thread);
#endif
    }
}

bool thread::operator==(const thread& other) const
{
#if defined(BOOST_HAS_WINTHREADS)
    return other.m_id == m_id;
#elif defined(BOOST_HAS_PTHREADS)
    return pthread_equal(m_thread, other.m_thread) != 0;
#endif
}

bool thread::operator!=(const thread& other) const
{
    return !operator==(other);
}

void thread::join()
{
    int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_thread), INFINITE);
    assert(res == WAIT_OBJECT_0);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_thread));
    assert(res);
#elif defined(BOOST_HAS_PTHREADS)
    res = pthread_join(m_thread, 0);
    assert(res == 0);
#endif
    // This isn't a race condition since any race that could occur would
    // have us in undefined behavior territory any way.
    m_joinable = false;
}

void thread::sleep(const xtime& xt)
{
#if defined(BOOST_HAS_WINTHREADS)
    unsigned milliseconds;
    to_duration(xt, milliseconds);
    Sleep(milliseconds);
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(BOOST_HAS_PTHREAD_DELAY_NP)
    timespec ts;
    to_timespec(xt, ts);
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
    semaphore sema;
    sema.down(xt);
#   endif
#endif
}

void thread::yield()
{
#if defined(BOOST_HAS_WINTHREADS)
    Sleep(0);
#elif defined(BOOST_HAS_PTHREADS)
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
#endif
}

thread_group::thread_group()
{
}

thread_group::~thread_group()
{
    // We shouldn't have to scoped_lock here, since referencing this object from another thread
    // while we're deleting it in the current thread is going to lead to undefined behavior
    // any way.
    for (std::list<thread*>::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
        delete (*it);
}

thread* thread_group::create_thread(const function0<void>& threadfunc)
{
    // No scoped_lock required here since the only "shared data" that's modified here occurs
    // inside add_thread which does scoped_lock.
    std::auto_ptr<thread> thrd(new thread(threadfunc));
    add_thread(thrd.get());
    return thrd.release();
}

void thread_group::add_thread(thread* thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to add a thread object multiple times.
    // Should we consider this an error and either throw or return an error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(), m_threads.end(), thrd);
    assert(it == m_threads.end());
    if (it == m_threads.end())
        m_threads.push_back(thrd);
}

void thread_group::remove_thread(thread* thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to remove a thread object that's not in the group.
    // Should we consider this an error and either throw or return an error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(), m_threads.end(), thrd);
    assert(it != m_threads.end());
    if (it != m_threads.end())
        m_threads.erase(it);
}

void thread_group::join_all()
{
    mutex::scoped_lock scoped_lock(m_mutex);
    for (std::list<thread*>::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
        (*it)->join();
}

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//    1 Jun 01  WEKEMPF Added boost::thread initial implementation.
//    3 Jul 01  WEKEMPF Redesigned boost::thread to be noncopyable.
