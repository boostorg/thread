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
#include <boost/type_traits/is_pointer.hpp>
#include <new>
#include <memory>
#include <cassert>
#include <functional>
#include <errno.h>

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
#if defined(BOOST_HAS_WINTHREADS)
	long id() const;
#else
	const void* id() const;
#endif

#if defined(BOOST_THREAD_PRIORITY_SCHEDULING)
	void set_scheduling_parameter(int policy, const sched_param& param);
	void get_scheduling_parameter(int& policy, sched_param& param) const;
#endif // BOOST_THREAD_PRIORITY_SCHEDULING

private:
    mutable boost::mutex m_mutex;
    boost::condition m_cond;
    boost::function0<void> m_threadfunc;
	unsigned int m_refcount;
	int m_state;
#if defined(BOOST_HAS_WINTHREADS)
    HANDLE m_thread;
	DWORD m_id;
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
	m_id = GetCurrentThreadId();
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
		m_id = GetCurrentThreadId();
#elif defined(BOOST_HAS_PTHREADS)
		m_thread = pthread_self();
#endif
		m_state = thread::data::running;
		m_cond.notify_all();
	}
    m_threadfunc();
}

#if defined(BOOST_HAS_WINTHREADS)
long thread::data::id() const
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_state != joined)
		return m_id;
	return 0; // throw instead?
}
#else
const void* thread::data::id() const
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_state != joined)
	{
		if (boost::is_pointer<pthread_t>::value)
			return m_thread;
		return this;
	}
	return 0; // throw instead?
}
#endif

#if defined(BOOST_THREAD_PRIORITY_SCHEDULING)

void thread::data::set_scheduling_parameter(int policy, const sched_param& param)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (policy != sched_other)
		throw std::invalid_argument("policy");
	if (param.priority < THREAD_PRIORITY_LOWEST || param.priority > THREAD_PRIORITY_HIGHEST)
		throw std::invalid_argument("param");
	boost::mutex::scoped_lock lock(m_mutex);
	BOOL res = FALSE;
	res = SetThreadPriority(m_thread, param.priority);
	assert(res);
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_setschedparam(m_thread, policy, &param);
	if (res == EINVAL || res == ENOTSUP)
		throw std::invalid_argument("policy/param");
	if (res == EPERM)
		throw std::runtime_error("permission denied");
	assert(res == 0);
#endif
}

void thread::data::get_scheduling_parameter(int& policy, sched_param& param) const
{
#if defined(BOOST_HAS_WINTHREADS)
	policy = sched_other;
	boost::mutex::scoped_lock lock(m_mutex);
	param.priority = GetThreadPriority(m_thread);
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	boost::mutex::scoped_lock lock(m_mutex);
	res = pthread_getschedparam(m_thread, &policy, &param);
	assert(res == 0);
#endif
}

#endif // BOOST_THREAD_PRIORITY_SCHEDULING

thread_cancel::thread_cancel()
{
}

thread_cancel::~thread_cancel()
{
}

thread::attributes::attributes()
{
#if defined(BOOST_HAS_WINTHREADS)
	m_stacksize = 0;
	m_schedinherit = true;
	m_schedparam.priority = THREAD_PRIORITY_NORMAL;
#elif defined(BOOST_HAS_PTHREADS)
	int res = pthread_attr_init(&m_attr);
	if (res == ENOMEM)
		throw thread_resource_error();
	assert(res == 0);
#endif
}

thread::attributes::~attributes()
{
#if defined(BOOST_HAS_PTHREADS)
	pthread_attr_destroy(&m_attr);
#endif
}

#if defined(BOOST_THREAD_ATTRIBUTES_STACKSIZE)

thread::attributes& thread::attributes::stack_size(size_t size)
{
#if defined(BOOST_HAS_WINTHREADS)
	m_stacksize = size;
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setstacksize(&m_attr, size);
	if (res == EINVAL)
		throw std::invalid_argument("size");
	assert(res == 0);
#endif
	return *this;
}

size_t thread::attributes::stack_size() const
{
#if defined(BOOST_HAS_WINTHREADS)
	return m_stacksize;
#elif defined(BOOST_HAS_PTHREADS)
	size_t size;
	int res = 0;
	res = pthread_attr_getstacksize(&m_attr, &size);
	assert(res == 0);
	return size;
#endif
}

#endif // BOOST_THREAD_ATTRIBUTES_STACKSIZE

#if defined(BOOST_THREAD_ATTRIBUTES_STACKADDR)

thread::attributes& thread::attributes::stack_address(void* addr)
{
#if defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setstackaddr(&m_attr, addr);
	assert(res == 0);
#endif
	return *this;
}

void* thread::attributes::stack_address() const
{
#if defined(BOOST_HAS_PTHREADS)
	void* addr;
	int res = 0;
	res = pthread_attr_getstackaddr(&m_attr, &addr);
	assert(res == 0);
	return addr;
#endif
}

#endif // BOOST_THREAD_ATTRIBUTES_STACKADDR

#if defined(BOOST_THREAD_PRIORITY_SCHEDULING)

thread::attributes& thread::attributes::inherit_scheduling(bool inherit)
{
#if defined(BOOST_HAS_WINTHREADS)
	m_schedinherit = inherit;
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setinheritsched(&m_attr, inherit ? PTHREAD_INHERIT_SCHED : PTHREAD_EXPLICIT_SCHED);
	if (res == ENOTSUP)
		throw std::invalid_argument("inherit");
	assert(res == 0);
#endif
	return *this;
}

bool thread::attributes::inherit_scheduling() const
{
#if defined(BOOST_HAS_WINTHREADS)
	return m_schedinherit;
#elif defined (BOOST_HAS_PTHREADS)
	int inherit = 0;
	int res = 0;
	res = pthread_attr_getinheritsched(&m_attr, &inherit);
	assert(res == 0);
	return inherit == PTHREAD_INHERIT_SCHED;
#endif
}

thread::attributes& thread::attributes::scheduling_parameter(const sched_param& param)
{
#if defined(BOOST_HAS_WINTHREADS)
	m_schedparam = param;
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setschedparam(&m_attr, &param);
	if (res == EINVAL || res == ENOTSUP)
		throw std::invalid_argument("param");
#endif
	return *this;
}

sched_param thread::attributes::scheduling_parameter() const
{
#if defined(BOOST_HAS_WINTHREADS)
	return m_schedparam;
#elif defined(BOOST_HAS_PTHREADS)
	sched_param param;
	int res = 0;
	res = pthread_attr_getschedparam(&m_attr, &param);
	assert(res == 0);
	return param;
#endif
}

thread::attributes& thread::attributes::scheduling_policy(int policy)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (policy != sched_other)
		throw std::invalid_argument("policy");
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setschedpolicy(&m_attr, policy);
	if (res == ENOTSUP)
		throw std::invalid_argument("policy");
	assert(res);
#endif
	return *this;
}

int thread::attributes::scheduling_policy() const
{
#if defined(BOOST_HAS_WINTHREADS)
	return sched_other;
#elif defined(BOOST_HAS_PTHREADS)
	int policy = 0;
	int res = 0;
	res = pthread_attr_getschedpolicy(&m_attr, &policy);
	assert(res == 0);
	return policy;
#endif
}

thread::attributes& thread::attributes::scope(int scope)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (scope != scope_system)
		throw std::invalid_argument("scope");
#elif defined(BOOST_HAS_PTHREADS)
	int res = 0;
	res = pthread_attr_setscope(&m_attr, scope);
	if (res == EINVAL || res == ENOTSUP)
		throw std::invalid_argument("scope");
	assert(res == 0);
#endif
	return *this;
}

int thread::attributes::scope() const
{
#if defined(BOOST_HAS_WINTHREADS)
	return scope_system;
#elif defined(BOOST_HAS_PTHREADS)
	int scope = 0;
	int res = 0;
	res = pthread_attr_getscope(&m_attr, &scope);
	return scope;
#endif
}

#endif // BOOST_THREAD_PRIORITY_SCHEDULING

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
    HANDLE h = (HANDLE)_beginthreadex(0, attr.m_stacksize, &thread_proxy, param.get(), CREATE_SUSPENDED, &id);
    if (!h)
        throw thread_resource_error();
	if (!attr.m_schedinherit)
		SetThreadPriority(h, attr.m_schedparam.priority);
	ResumeThread(h);
#elif defined(BOOST_HAS_PTHREADS)
    int res = 0;
	pthread_t t;
    res = pthread_create(&t, &attr.m_attr, &thread_proxy, param.get());
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

thread::thread(const thread& other)
	: m_handle(other.m_handle)
{
	m_handle->addref();
}

thread::~thread()
{
	if (m_handle && m_handle->release())
		delete m_handle;
}

thread& thread::operator=(const thread& other)
{
	if (m_handle->release())
		delete m_handle;
	m_handle = other.m_handle;
	m_handle->addref();
	return *this;
}

bool thread::operator==(const thread& other) const
{
	return m_handle == other.m_handle;
}

bool thread::operator!=(const thread& other) const
{
    return !operator==(other);
}

bool thread::operator<(const thread& other) const
{
	return std::less<thread::data*>()(m_handle, m_handle);
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

#if defined(BOOST_THREAD_PRIORITY_SCHEDULING)

void thread::set_scheduling_parameter(int policy, const sched_param& param)
{
	m_handle->set_scheduling_parameter(policy, param);
}

void thread::get_scheduling_parameter(int& policy, sched_param& param) const
{
	m_handle->get_scheduling_parameter(policy, param);
}

int thread::max_priority(int policy)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (policy != sched_other)
		throw std::invalid_argument("policy");
	return THREAD_PRIORITY_HIGHEST;
#elif defined(BOOST_HAS_PTHREADS)
#endif
	return 0;
}

int thread::min_priority(int policy)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (policy != sched_other)
		throw std::invalid_argument("policy");
	return THREAD_PRIORITY_LOWEST;
#elif defined(BOOST_HAS_PTHREADS)
#endif
	return 0;
}

#endif // BOOST_THREAD_PRIORITY_SCHEDULING

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

#if defined(BOOST_HAS_WINTHREADS)
long thread::id() const
#else
const void* thread::id() const
#endif
{
	std::cout << *this;
	return m_handle->id();
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
