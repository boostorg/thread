// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/win32/config.hpp>

#include <boost/thread/win32/thread.hpp>
#include <boost/thread/win32/xtime.hpp>
#include <boost/thread/win32/condition.hpp>
#include <cassert>


#include <windows.h>
#if !defined(BOOST_NO_THREADEX)
#   include <process.h>
#endif

#include "timeconv.inl"

//#include "boost/thread/win32/tss_hooks.hpp"

namespace {

#if defined(BOOST_NO_THREADEX)
// Windows CE doesn't define _beginthreadex

struct ThreadProxyData
{
    typedef unsigned (__stdcall* func)(void*);
    func start_address_;
    void* arglist_;
    ThreadProxyData(func start_address,void* arglist) : start_address_(start_address), arglist_(arglist) {}
};

DWORD WINAPI ThreadProxy(LPVOID args)
{
    ThreadProxyData* data=reinterpret_cast<ThreadProxyData*>(args);
    DWORD ret=data->start_address_(data->arglist_);
    delete data;
    return ret;
}

inline unsigned _beginthreadex(void* security, unsigned stack_size, unsigned (__stdcall* start_address)(void*),
void* arglist, unsigned initflag,unsigned* thrdaddr)
{
    DWORD threadID;
    HANDLE hthread=CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(security),stack_size,ThreadProxy,
        new ThreadProxyData(start_address,arglist),initflag,&threadID);
    if (hthread!=0)
        *thrdaddr=threadID;
    return reinterpret_cast<unsigned>(hthread);
}
#endif

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
    unsigned __stdcall thread_proxy(void* param)
    {
        try
        {
            thread_param* p = static_cast<thread_param*>(param);
            boost::function0<void> threadfunc = p->m_threadfunc;
            p->started();
            threadfunc();
//            on_thread_exit();
        }
        catch (...)
        {
//            on_thread_exit();
        }
        return 0;
    }

}

namespace boost {

thread::thread()
    : m_joinable(false)
{
    m_thread = reinterpret_cast<void*>(GetCurrentThread());
    m_id = GetCurrentThreadId();
}

thread::thread(const function0<void>& threadfunc)
    : m_joinable(true)
{
    thread_param param(threadfunc);
    m_thread = reinterpret_cast<void*>(_beginthreadex(0, 0, &thread_proxy,
                                           &param, 0, &m_id));
    if (!m_thread)
        throw thread_resource_error();
    param.wait();
}

thread::~thread()
{
    if (m_joinable)
    {
        int res = 0;
        res = CloseHandle(reinterpret_cast<HANDLE>(m_thread));
        assert(res);
    }
}

bool thread::operator==(const thread& other) const
{
    return other.m_id == m_id;
}

bool thread::operator!=(const thread& other) const
{
    return !operator==(other);
}

void thread::join()
{
    assert(m_joinable); //See race condition comment below
    int res = 0;
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_thread), INFINITE);
    assert(res == WAIT_OBJECT_0);
    res = CloseHandle(reinterpret_cast<HANDLE>(m_thread));
    assert(res);
    // This isn't a race condition since any race that could occur would
    // have us in undefined behavior territory any way.
    m_joinable = false;
}

void thread::sleep(const xtime& xt)
{
    for (int foo=0; foo < 5; ++foo)
    {
        int milliseconds;
        to_duration(xt, milliseconds);
        Sleep(milliseconds);
        xtime cur;
        xtime_get(&cur, TIME_UTC);
        if (xtime_cmp(xt, cur) <= 0)
            return;
    }
}

void thread::yield()
{
    Sleep(0);
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

int thread_group::size()
{
        return m_threads.size();
}

} // namespace boost
