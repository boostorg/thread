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
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <process.h>
#endif

#include "timeconv.inl"

#if defined(BOOST_HAS_PTHREADS)
namespace boost {

// This class is used to signal thread objects when the thread dies.
class thread::thread_list
{
public:
    thread_list() { }
    ~thread_list()
    {
        mutex::lock lock(m_mutex);
        for (std::list<thread*>::iterator it = m_thread_objects.begin(); it != m_thread_objects.end(); ++it)
        {
            mutex::lock lock((*it)->m_mutex);
            (*it)->m_state_manager = 0;
            (*it)->m_condition.notify_all();
        }
    }

    void add(thread* thrd)
    {
        mutex::lock lock(m_mutex);
        m_thread_objects.push_back(thrd);
    }

    void remove(thread* thrd)
    {
        mutex::lock lock(m_mutex);
        std::list<thread*>::iterator it = std::find(m_thread_objects.begin(), m_thread_objects.end(), thrd);
        if (it != m_thread_objects.end())
            m_thread_objects.erase(it);
    }

private:
    std::list<thread*> m_thread_objects;
    mutex m_mutex;
};

} // namespace boost
#endif

namespace {

#if defined(BOOST_HAS_PTHREADS)
pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;

void destroy_list(void* p)
{
    boost::thread::thread_list* list = static_cast<boost::thread::thread_list*>(p);
    delete list;
}

void init_key()
{
    int res = pthread_key_create(&key, &destroy_list);
    assert(res == 0);
}

pthread_key_t get_key()
{
    int res = pthread_once(&once, &init_key);
    assert(res == 0);
    return key;
}

boost::thread::thread_list* get_list()
{
    pthread_key_t key = get_key();
    boost::thread::thread_list* list = static_cast<boost::thread::thread_list*>(pthread_getspecific(key));
    if (!list)
    {
        list = new boost::thread::thread_list;
        pthread_setspecific(key, list);
    }
    return list;
}
#endif

class thread_param
{
public:
    thread_param(const boost::function0<void>& threadfunc) : m_threadfunc(threadfunc), m_started(false) { }
    void wait()
    {
        boost::mutex::lock lock(m_mutex);
        while (!m_started)
            m_condition.wait(lock);
    }
    void started()
    {
        boost::mutex::lock lock(m_mutex);
        m_started = true;
        m_condition.notify_one();
    }

    boost::mutex m_mutex;
    boost::condition m_condition;
    const boost::function0<void>& m_threadfunc;
    bool m_started;
#if defined(BOOST_HAS_PTHREADS)
    boost::thread::thread_list* m_state_manager;
#endif
};

#if defined(BOOST_HAS_WINTHREADS)
unsigned __stdcall thread_proxy(void* param)
#elif defined(BOOST_HAS_PTHREADS)
void* thread_proxy(void* param)
#endif
{
    thread_param* p = static_cast<thread_param*>(param);
    boost::function0<void> threadfunc = p->m_threadfunc;
#if defined(BOOST_HAS_PTHREADS)
    p->m_state_manager = get_list(); // create the list
#endif
    p->started();
    threadfunc();
    return 0;
}

} // unnamed namespace

namespace boost {

lock_error::lock_error() : std::runtime_error("thread lock error")
{
}

thread::thread()
{
#if defined(BOOST_HAS_WINTHREADS)
    HANDLE cur = GetCurrentThread();
    HANDLE real;
    DuplicateHandle(GetCurrentProcess(), cur, GetCurrentProcess(), &real, 0, FALSE, DUPLICATE_SAME_ACCESS);
    m_thread = reinterpret_cast<unsigned long>(real);
    m_id = GetCurrentThreadId();
#elif defined(BOOST_HAS_PTHREADS)
    m_thread = pthread_self();
    m_state_manager = get_list();
    m_state_manager->add(this);
#endif
}

thread::thread(const function0<void>& threadfunc)
{
    thread_param param(threadfunc);
#if defined(BOOST_HAS_WINTHREADS)
    m_thread = _beginthreadex(0, 0, &thread_proxy, &param, 0, &m_id);
    assert(m_thread);
#elif defined(BOOST_HAS_PTHREADS)
    int res = pthread_create(&m_thread, 0, &thread_proxy, &param);
    assert(res == 0);
#endif
    param.wait();
#if defined(BOOST_HAS_PTHREADS)
    m_state_manager = param.m_state_manager;
    assert(m_state_manager);
    m_state_manager->add(this);
#endif
}

thread::~thread()
{
    int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
    res = CloseHandle(reinterpret_cast<HANDLE>(m_thread));
    assert(res);
#elif defined(BOOST_HAS_PTHREADS)
    {
        mutex::lock lock(m_mutex);
        if (m_state_manager)
            m_state_manager->remove(this);
    }

    res = pthread_detach(m_thread);
    assert(res == 0);
#endif
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
    return operator!=(other);
}

void thread::join()
{
    int res;
#if defined(BOOST_HAS_WINTHREADS)
    res = WaitForSingleObject(reinterpret_cast<HANDLE>(m_thread), INFINITE);
    assert(res == WAIT_OBJECT_0);
#elif defined(BOOST_HAS_PTHREADS)
    mutex::lock lock(m_mutex);
    while (m_state_manager)
        m_condition.wait(lock);
#endif
}

bool thread::try_join()
{
#if defined(BOOST_HAS_WINTHREADS)
    return WaitForSingleObject(reinterpret_cast<HANDLE>(m_thread), 0) == WAIT_OBJECT_0;
#elif defined(BOOST_HAS_PTHREADS)
    mutex::lock lock(m_mutex);
    bool ret = (m_state_manager == 0);
    return ret;
#endif
}

bool thread::timed_join(const xtime& xt)
{
#if defined(BOOST_HAS_WINTHREADS)
    unsigned milliseconds;
    to_duration(xt, milliseconds);
    return WaitForSingleObject(reinterpret_cast<HANDLE>(m_thread), 0) == WAIT_OBJECT_0;
#elif defined(BOOST_HAS_PTHREADS)
    mutex::lock lock(m_mutex);
    while (m_state_manager)
    {
        if (!m_condition.timed_wait(lock, xt))
            break;
    }
    bool ret = (m_state_manager == 0);
    return ret;
#endif
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
    int res = pthread_delay_np(&ts);
    assert(res == 0);
#   elif defined(BOOST_HAS_NANOSLEEP)
    timespec ts;
    to_timespec(xt, ts);
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
    int res = sched_yield();
    assert(res == 0);
#   elif defined(BOOST_HAS_PTHREAD_YIELD)
    int res = pthread_yield();
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
    // We shouldn't have to lock here, since referencing this object from another thread
    // while we're deleting it in the current thread is going to lead to undefined behavior
    // any way.
    for (std::list<thread*>::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
        delete (*it);
}

thread* thread_group::create_thread(const function0<void>& threadfunc)
{
    // No lock required here since the only "shared data" that's modified here occurs
    // inside add_thread which does lock.
    std::auto_ptr<thread> thrd(new thread(threadfunc));
    add_thread(thrd.get());
    return thrd.release();
}

void thread_group::add_thread(thread* thrd)
{
    mutex::lock lock(m_mutex);

    // For now we'll simply ignore requests to add a thread object multiple times.
    // Should we consider this an error and either throw or return an error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(), m_threads.end(), thrd);
    assert(it == m_threads.end());
    if (it == m_threads.end())
        m_threads.push_back(thrd);
}

void thread_group::remove_thread(thread* thrd)
{
    mutex::lock lock(m_mutex);

    // For now we'll simply ignore requests to remove a thread object that's not in the group.
    // Should we consider this an error and either throw or return an error value?
    std::list<thread*>::iterator it = std::find(m_threads.begin(), m_threads.end(), thrd);
    assert(it != m_threads.end());
    if (it != m_threads.end())
        m_threads.erase(it);
}

void thread_group::join_all()
{
    mutex::lock lock(m_mutex);
    for (std::list<thread*>::iterator it = m_threads.begin(); it != m_threads.end(); ++it)
        (*it)->join();
}

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//    1 Jun 01  WEKEMPF Added boost::thread initial implementation.
//    3 Jul 01  WEKEMPF Redesigned boost::thread to be noncopyable.
