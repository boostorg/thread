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
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/tss.hpp>
#include <new>
#include <memory>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <process.h>
#elif defined(BOOST_HAS_MPTASKS)
#   include <DriverServices.h>
#   include "init.hpp"
#   include "safe.hpp"
#endif

#include "timeconv.inl"

namespace boost {

class thread::data
{
public:
	enum
	{
		creating,
		running,
		joining,
		joined
	};

    data(const boost::function0<void>& threadfunc);
	data();
	~data();

	void addref();
	bool release();
	void join();
	void cancel();
	void test_cancel();
	void run();

private:
    boost::mutex m_mutex;
    boost::condition m_cond;
    boost::function0<void> m_threadfunc;
	unsigned int m_refcount;
	int m_state;
#if defined(BOOST_HAS_WINTHREADS)
    HANDLE m_thread;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_t m_thread;
#elif defined(BOOST_HAS_MPTASKS)
    MPQueueID m_pJoinQueueID;
    MPTaskID m_pTaskID;
#endif
	bool m_canceled;
};

} // namespace boost

namespace {

void release_tss_data(boost::thread::data* data)
{
	assert(data);
	if (data->release())
		delete data;
}

boost::thread_specific_ptr<boost::thread::data> tss_thread_data(&release_tss_data);

struct thread_equals
{
	thread_equals(boost::thread& thrd) : m_thrd(thrd) { }
	bool operator()(boost::thread* thrd) { return *thrd == m_thrd; }
	boost::thread& m_thrd;
};

} // unnamed namespace

extern "C" {

#if defined(BOOST_HAS_WINTHREADS)
unsigned __stdcall thread_proxy(void* param)
#elif defined(BOOST_HAS_PTHREADS)
static void* thread_proxy(void* param)
#elif defined(BOOST_HAS_MPTASKS)
static OSStatus thread_proxy(void* param)
#endif
{
    try
    {
		boost::thread::data* tdata = static_cast<boost::thread::data*>(param);
		tss_thread_data.reset(tdata);
		tdata->run();
    }
	catch (boost::thread_cancel)
	{
	}
    catch (...)
    {
		std::terminate();
    }
#if defined(BOOST_HAS_MPTASKS)
    ::boost::detail::thread_cleanup();
#endif
    return 0;
}

} // extern "C"

namespace boost {

thread::data::data(const boost::function0<void>& threadfunc)
	: m_threadfunc(threadfunc), m_refcount(2), m_state(creating), m_canceled(false)
{
}

thread::data::data()
	: m_refcount(2), m_state(running), m_canceled(false)
{
#if defined(BOOST_HAS_WINTHREADS)
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
		&m_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
#elif defined(BOOST_HAS_PTHREADS)
	m_thread = pthread_self();
#endif
}

thread::data::~data()
{
	boost::mutex::scoped_lock lock(m_mutex);

	if (m_state != joined)
	{
		lock.unlock();

		int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
		res = CloseHandle(m_thread);
		assert(res);
#elif defined(BOOST_HAS_PTHREADS)
		res = pthread_detach(m_thread);
		assert(res == 0);
#elif defined(BOOST_HAS_MPTASKS)
		OSStatus lStatus = threads::mac::detail::safe_wait_on_queue(m_pJoinQueueID, NULL, NULL, NULL, kDurationForever);
		assert(lStatus == noErr);
#endif

		lock.lock();
		m_state = joined;
		m_cond.notify_all();
	}
}

void thread::data::addref()
{
	boost::mutex::scoped_lock lock(m_mutex);
	++m_refcount;
}

bool thread::data::release()
{
	boost::mutex::scoped_lock lock(m_mutex);
	return (--m_refcount == 0);
}

void thread::data::join()
{
	boost::mutex::scoped_lock lock(m_mutex);

	while (m_state == creating || m_state == joining)
		m_cond.wait(lock);

	if (m_state != joined)
	{
		m_state = joining;
		lock.unlock();

		int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
		res = WaitForSingleObject(m_thread, INFINITE);
		assert(res == WAIT_OBJECT_0);
		res = CloseHandle(m_thread);
		assert(res);
#elif defined(BOOST_HAS_PTHREADS)
		res = pthread_join(m_thread, 0);
		assert(res == 0);
#elif defined(BOOST_HAS_MPTASKS)
		OSStatus lStatus = threads::mac::detail::safe_wait_on_queue(m_pJoinQueueID, NULL, NULL, NULL, kDurationForever);
		assert(lStatus == noErr);
#endif

		lock.lock();
		m_state = joined;
		m_cond.notify_all();
	}
}

void thread::data::cancel()
{
	boost::mutex::scoped_lock lock(m_mutex);
	m_canceled = true;
}

void thread::data::test_cancel()
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_canceled)
		throw boost::thread_cancel();
}

void thread::data::run()
{
	{
		boost::mutex::scoped_lock lock(m_mutex);
#if defined(BOOST_HAS_WINTHREADS)
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
			&m_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
#elif defined(BOOST_HAS_PTHREADS)
		m_thread = pthread_self();
#endif
		m_state = thread::data::running;
		m_cond.notify_all();
	}
    m_threadfunc();
}

thread_cancel::thread_cancel()
{
}

thread_cancel::~thread_cancel()
{
}

thread::attributes::attributes()
{
}

thread::attributes::~attributes()
{
}

thread::thread()
    : m_handle(0)
{
#if defined(BOOST_HAS_MPTASKS)
    threads::mac::detail::thread_init();
    threads::mac::detail::create_singletons();
    m_pTaskID = MPCurrentTaskID();
    m_pJoinQueueID = kInvalidID;
#endif
	thread::data* tdata = tss_thread_data.get();
	if (tdata == 0)
	{
		tdata = new(std::nothrow) thread::data;
		if (!tdata)
			throw thread_resource_error();
		tss_thread_data.reset(tdata);
	}
	else
		tdata->addref();
	m_handle = tdata;
}

thread::thread(const function0<void>& threadfunc, attributes attr)
    : m_handle(0)
{
	std::auto_ptr<thread::data> param(new(std::nothrow) thread::data(threadfunc));
	if (param.get() == 0)
		throw thread_resource_error();
#if defined(BOOST_HAS_WINTHREADS)
	unsigned int id;
    HANDLE h = (HANDLE)_beginthreadex(0, 0, &thread_proxy,	param.get(), 0, &id);
    if (!h)
        throw thread_resource_error();
#elif defined(BOOST_HAS_PTHREADS)
    int res = 0;
	pthread_t t;
    res = pthread_create(&t, 0, &thread_proxy, param.get());
    if (res != 0)
        throw thread_resource_error();
#elif defined(BOOST_HAS_MPTASKS)
    threads::mac::detail::thread_init();
    threads::mac::detail::create_singletons();
    OSStatus lStatus = noErr;

    m_pJoinQueueID = kInvalidID;
    m_pTaskID = kInvalidID;

    lStatus = MPCreateQueue(&m_pJoinQueueID);
    if(lStatus != noErr) throw thread_resource_error();

    lStatus = MPCreateTask(&thread_proxy, param.get(), 0UL, m_pJoinQueueID, NULL, NULL,
                            0UL, &m_pTaskID);
    if(lStatus != noErr)
    {
        lStatus = MPDeleteQueue(m_pJoinQueueID);
        assert(lStatus == noErr);
        throw thread_resource_error();
    }
#endif
	m_handle = param.release();
}

thread::~thread()
{
	if (m_handle && m_handle->release())
		delete m_handle;
}

bool thread::operator==(const thread& other) const
{
	return m_handle == other.m_handle;
}

bool thread::operator!=(const thread& other) const
{
    return !operator==(other);
}

void thread::join()
{
	m_handle->join();
}

void thread::cancel()
{
	m_handle->cancel();
}

void thread::test_cancel()
{
	thread self;
	self.m_handle->test_cancel();
}

void thread::sleep(const xtime& xt)
{
    for (;;)
    {
		thread::test_cancel();
#if defined(BOOST_HAS_WINTHREADS)
        int milliseconds;
        to_duration(xt, milliseconds);
        Sleep(milliseconds);
#elif defined(BOOST_HAS_PTHREADS)
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
#elif defined(BOOST_HAS_MPTASKS)
        int microseconds;
        to_microduration(xt, microseconds);
        Duration lMicroseconds(kDurationMicrosecond * microseconds);
        AbsoluteTime sWakeTime(DurationToAbsolute(lMicroseconds));
        threads::mac::detail::safe_delay_until(&sWakeTime);
#endif
        xtime cur;
        xtime_get(&cur, TIME_UTC);
        if (xtime_cmp(xt, cur) <= 0)
            return;
    }
	thread::test_cancel();
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
#elif defined(BOOST_HAS_MPTASKS)
    MPYield();
#endif
	thread::test_cancel();
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

thread* thread_group::find(thread& thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to remove a thread object that's not in the group.
    // Should we consider this an error and either throw or return an error value?
	std::list<thread*>::iterator it = std::find_if(m_threads.begin(), m_threads.end(), thread_equals(thrd));
    if (it != m_threads.end())
		return *it;

	return 0;
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
