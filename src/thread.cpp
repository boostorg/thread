// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/tss.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/mpl/if.hpp>
#include <new>
#include <memory>
#include <exception>
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

namespace {

struct pointer_based
{
    template <typename T>
    static const void* do_from(const T& obj) { return obj; }
};

struct value_based
{
    template <typename T>
    static const void* do_from(const T& obj) { return 0; }
};

template <typename T>
struct as_pointer : private boost::mpl::if_<boost::is_pointer<T>,
    pointer_based, value_based>::type
{
    static const void* from(const T& obj) { return do_from(obj); }
};

class thread_data
{
public:
    enum
    {
        creating,
        running,
        finished,
        joined
    };

    thread_data(const boost::function0<void>& threadfunc);
    thread_data();
    ~thread_data();

    void addref();
    bool release();
    void join();
    bool timed_join(const boost::xtime& xt);
    void cancel();
    bool cancelled() const;
    void enable_cancellation();
    void disable_cancellation();
    void test_cancel();
    void run();
    boost::thread::id_type id() const;

    void set_scheduling_parameter(int policy, const boost::sched_param& param);
    void get_scheduling_parameter(int& policy,
        boost::sched_param& param) const;

    static thread_data* get_current();

private:
    mutable boost::mutex m_mutex;
    mutable boost::condition m_cond;
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
    bool m_cancelled;
    int m_cancellation_disabled_level;
    bool m_native;
};

void release_tss_data(thread_data* data)
{
    assert(data);
    if (data->release())
        delete data;
}

boost::thread_specific_ptr<thread_data> tss_thread_data(&release_tss_data);
//:There are problems with this being a global static:
//:If the user creates a thread object before global statics
//:have been initialized--e.g. in dllmain--then this
//:won't have been initialized; BANG!
//:Perhaps use Boost.Threads once instead?

thread_data::thread_data(const boost::function0<void>& threadfunc)
    : m_threadfunc(threadfunc), m_refcount(2), m_state(creating),
      m_cancelled(false), m_cancellation_disabled_level(0), m_native(false)
{
}

thread_data::thread_data()
    : m_refcount(1), m_state(running), m_cancelled(false),
      m_cancellation_disabled_level(0), m_native(true)
{
#if defined(BOOST_HAS_WINTHREADS)
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
        GetCurrentProcess(), &m_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    m_id = GetCurrentThreadId();
#elif defined(BOOST_HAS_PTHREADS)
    m_thread = pthread_self();
#endif
}

thread_data::~thread_data()
{
    if (m_state != joined)
    {
        int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
        res = CloseHandle(m_thread);
        assert(res);
#elif defined(BOOST_HAS_PTHREADS)
        if (!m_native)
        {
            res = pthread_detach(m_thread);
            assert(res == 0);
        }
#elif defined(BOOST_HAS_MPTASKS)
        OSStatus lStatus =
            threads::mac::detail::safe_wait_on_queue(m_pJoinQueueID,
                NULL, NULL, NULL, kDurationForever);
        assert(lStatus == noErr);
#endif
    }
}

void thread_data::addref()
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);
    ++m_refcount;
}

bool thread_data::release()
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);
    return (--m_refcount == 0);
}

void thread_data::join()
{
    bool do_join=false;
    {
        boost::mutex::scoped_lock lock(m_mutex);
        while (m_state != joined && m_state != finished)
            m_cond.wait(lock);
        do_join = (m_state == finished);
    }
    if (do_join)
    {
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
        OSStatus lStatus =
            threads::mac::detail::safe_wait_on_queue(m_pJoinQueueID, NULL,
                NULL, NULL,
                kDurationForever);
        assert(lStatus == noErr);
#endif
        boost::mutex::scoped_lock lock(m_mutex);
        m_state = joined;
        m_cond.notify_all();
    }
}

bool thread_data::timed_join(const boost::xtime& xt)
{
    bool do_join=false;
    {
        boost::mutex::scoped_lock lock(m_mutex);
        while (m_state != joined && m_state != finished)
        {
            if (!m_cond.timed_wait(lock, xt))
                return false;
        }
    }
    if (do_join)
    {
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
        OSStatus lStatus =
            threads::mac::detail::safe_wait_on_queue(m_pJoinQueueID, NULL,
                NULL, NULL,
                kDurationForever);
        assert(lStatus == noErr);
#endif
        boost::mutex::scoped_lock lock(m_mutex);
        m_state = joined;
        m_cond.notify_all();
    }
    return true;
}

void thread_data::cancel()
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);
    m_cancelled = true;
}

bool thread_data::cancelled() const
{
    boost::mutex::scoped_lock lock(m_mutex);
    return m_cancelled;
}

void thread_data::test_cancel()
{
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_cancellation_disabled_level == 0 && m_cancelled)
        throw boost::thread_cancel();
}

void thread_data::disable_cancellation()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_cancellation_disabled_level++;
}

void thread_data::enable_cancellation()
{
    boost::mutex::scoped_lock lock(m_mutex);
    m_cancellation_disabled_level--;
}

void thread_data::run()
{
    {
        boost::mutex::scoped_lock lock(m_mutex);
#if defined(BOOST_HAS_WINTHREADS)
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &m_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_id = GetCurrentThreadId();
#elif defined(BOOST_HAS_PTHREADS)
        m_thread = pthread_self();
#endif
        m_state = running;
        m_cond.notify_all();
    }
    try
    {
        m_threadfunc();
    }
    catch (boost::thread_cancel)
    {
    }
    boost::mutex::scoped_lock lock(m_mutex);
    m_state = finished;
    m_cond.notify_all();
}

boost::thread::id_type thread_data::id() const
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);

    if (m_state != joined)
#if defined(BOOST_HAS_WINTHREADS)
        return m_id;
#elif defined(BOOST_HAS_PTHREADS)
    {
        const void* res = as_pointer<pthread_t>::from(m_thread);
        if (res == 0)
            res = this;
        return res;
    }
#endif

    return 0; // throw instead?
}

void thread_data::set_scheduling_parameter(int policy,
    const boost::sched_param& param)
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);

#if defined(BOOST_HAS_WINTHREADS)
    if (policy != boost::sched_other)
        throw boost::invalid_thread_argument();
    if (param.priority < THREAD_PRIORITY_LOWEST ||
        param.priority > THREAD_PRIORITY_HIGHEST)
    {
        throw boost::invalid_thread_argument();
    }
    BOOL res = FALSE;
    res = SetThreadPriority(m_thread, param.priority);
    if (res == ERROR_ACCESS_DENIED)  // guessing about possible return value
        throw boost::thread_permission_error(res);
    assert(res);
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_setschedparam(m_thread, policy, &param);
    if (res == EINVAL)
        throw boost::invalid_thread_argument(res);
    if (res == ENOTSUP)
        throw boost::unsupported_thread_option(res);
    if (res == EPERM)
        throw boost::thread_permission_error(res);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

void thread_data::get_scheduling_parameter(int& policy,
    boost::sched_param& param) const
{
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_state == creating)
        m_cond.wait(lock);

#if defined(BOOST_HAS_WINTHREADS)
    policy = boost::sched_other;
    param.priority = GetThreadPriority(m_thread);
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_getschedparam(m_thread, &policy, &param);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

thread_data* thread_data::get_current()
{
    thread_data* data = tss_thread_data.get();
    if (data == 0)
    {
        data = new thread_data;
        if (!data)
            throw std::bad_alloc();
        tss_thread_data.reset(data);
    }
    return data;
}

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
            thread_data* data = static_cast<thread_data*>(param);
            tss_thread_data.reset(data);
            data->run();
        }
        catch (...)
        {
            using namespace std;
            terminate();
        }
#if defined(BOOST_HAS_MPTASKS)
        ::boost::detail::thread_cleanup();
#endif
        return 0;
    }

} // extern "C"

namespace boost {

thread_cancel::thread_cancel()
{
}

thread_cancel::~thread_cancel()
{
}

cancellation_guard::cancellation_guard()
{
    thread_data* data = thread_data::get_current();
    m_handle = data;
    data->disable_cancellation();
}

cancellation_guard::~cancellation_guard()
{
    static_cast<thread_data*>(m_handle)->enable_cancellation();
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
        throw thread_resource_error(res);
    assert(res == 0);
#endif
}

thread::attributes::~attributes()
{
#if defined(BOOST_HAS_PTHREADS)
    pthread_attr_destroy(&m_attr);
#endif
}

thread::attributes& thread::attributes::set_stack_size(size_t size)
{
#if defined(BOOST_HAS_WINTHREADS)
    m_stacksize = size;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_ATTR_STACKSIZE)
    int res = 0;
    res = pthread_attr_setstacksize(&m_attr, size);
    if (res == EINVAL)
        throw invalid_thread_argument(res);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
    return *this;
}

size_t thread::attributes::get_stack_size() const
{
#if defined(BOOST_HAS_WINTHREADS)
    return m_stacksize;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_ATTR_STACKSIZE)
    size_t size;
    int res = 0;
    res = pthread_attr_getstacksize(&m_attr, &size);
    assert(res == 0);
    return size;
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

thread::attributes& thread::attributes::set_stack_address(void* addr)
{
#if defined(BOOST_HAS_WINTHREADS)
    throw unsupported_thread_option();
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_ATTR_STACKADDR)
    int res = 0;
    res = pthread_attr_setstackaddr(&m_attr, addr);
    assert(res == 0);
    return *this;
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

void* thread::attributes::get_stack_address() const
{
#if defined(BOOST_HAS_WINTHREADS)
    throw unsupported_thread_option();
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_ATTR_STACKADDR)
    void* addr;
    int res = 0;
    res = pthread_attr_getstackaddr(&m_attr, &addr);
    assert(res == 0);
    return addr;
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

thread::attributes& thread::attributes::inherit_scheduling(bool inherit)
{
#if defined(BOOST_HAS_WINTHREADS)
    m_schedinherit = inherit;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_attr_setinheritsched(&m_attr,
        inherit ? PTHREAD_INHERIT_SCHED : PTHREAD_EXPLICIT_SCHED);
    if (res == ENOTSUP)
        throw invalid_thread_argument(res);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
    return *this;
}

bool thread::attributes::inherit_scheduling() const
{
#if defined(BOOST_HAS_WINTHREADS)
    return m_schedinherit;
#elif defined (BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int inherit = 0;
    int res = 0;
    res = pthread_attr_getinheritsched(&m_attr, &inherit);
    assert(res == 0);
    return inherit == PTHREAD_INHERIT_SCHED;
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

thread::attributes& thread::attributes::set_schedule(int policy,
    const sched_param& param)
{
#if defined(BOOST_HAS_WINTHREADS)
    if (policy != sched_other)
        throw unsupported_thread_option();
    if (param.priority < THREAD_PRIORITY_LOWEST ||
        param.priority > THREAD_PRIORITY_HIGHEST)
    {
        throw invalid_thread_argument();
    }
    m_schedparam = param;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_attr_setschedpolicy(&m_attr, policy);
    if (res == EINVAL)
        throw invalid_thread_argument(res);
    if (res == ENOTSUP)
        throw unsupported_thread_option(res);
    assert(res);
    res = pthread_attr_setschedparam(&m_attr, &param);
    // This one leaves me puzzled.  POSIX clearly indicates this can return
    // EINVAL if the sched_param supplied is invalid.  But you don't know if
    // it's invalid unless you know what policy it's meant for.  This leaves
    // us with a chicken and the egg dillema, but I'm going to assume we
    // should set the policy first (which won't return EINVAL based on current
    // parameter), then we set the parameter, which may return EINVAL if the
    // current policy indicates so. Big assumption... does anyone know the
    // definative answer?
    if (res == EINVAL)
        throw invalid_thread_argument(res);
    if (res == ENOTSUP)
        throw unsupported_thread_option(res);
    assert(res);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
    return *this;
}

void thread::attributes::get_schedule(int& policy, sched_param& param)
{
#if defined(BOOST_HAS_WINTHREADS)
    policy = sched_other;
    param = m_schedparam;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_attr_getschedpolicy(&m_attr, &policy);
    assert(res == 0);
    res = pthread_attr_getschedparam(&m_attr, &param);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
}

thread::attributes& thread::attributes::scope(int scope)
{
#if defined(BOOST_HAS_WINTHREADS)
    if (scope != scope_system)
        throw invalid_thread_argument();
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int res = 0;
    res = pthread_attr_setscope(&m_attr, scope);
    if (res == EINVAL)
        throw invalid_thread_argument(res);
    if (res == ENOTSUP)
        throw unsupported_thread_option(res);
    assert(res == 0);
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
    return *this;
}

int thread::attributes::scope() const
{
#if defined(BOOST_HAS_WINTHREADS)
    return scope_system;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    int scope = 0;
    int res = 0;
    res = pthread_attr_getscope(&m_attr, &scope);
    return scope;
#   else
    throw unsupported_thread_option(ENOTSUP);
#   endif
#endif
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
    thread_data* tdata = thread_data::get_current();
    tdata->addref();
    m_handle = tdata;
}

thread::thread(const function0<void>& threadfunc,
    const thread::attributes& attr)
    : m_handle(0)
{
    std::auto_ptr<thread_data> param(new thread_data(threadfunc));
    if (param.get() == 0)
        throw thread_resource_error();
#if defined(BOOST_HAS_WINTHREADS)
    unsigned int id;
    HANDLE h = (HANDLE)_beginthreadex(0, attr.m_stacksize, &thread_proxy,
        param.get(), CREATE_SUSPENDED, &id);
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
        throw thread_resource_error(res);
#elif defined(BOOST_HAS_MPTASKS)
    threads::mac::detail::thread_init();
    threads::mac::detail::create_singletons();
    OSStatus lStatus = noErr;

    m_pJoinQueueID = kInvalidID;
    m_pTaskID = kInvalidID;

    lStatus = MPCreateQueue(&m_pJoinQueueID);
    if (lStatus != noErr)
        throw thread_resource_error();

    lStatus = MPCreateTask(&thread_proxy, param.get(), 0UL, m_pJoinQueueID,
        NULL, NULL, 0UL, &m_pTaskID);
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
    static_cast<thread_data*>(m_handle)->addref();
}

thread::~thread()
{
    if (m_handle && static_cast<thread_data*>(m_handle)->release())
        delete static_cast<thread_data*>(m_handle);
}

thread& thread::operator=(const thread& other)
{
    thread_data* data = static_cast<thread_data*>(m_handle);
    if (data->release())
        delete data;
    m_handle = other.m_handle;
    static_cast<thread_data*>(m_handle)->addref();
    return *this;
}

bool thread::operator==(const thread& other) const
{
    return m_handle == other.m_handle;
}

bool thread::operator!=(const thread& other) const
{
    return m_handle != other.m_handle;
}

bool thread::operator<(const thread& other) const
{
    return std::less<void*>()(m_handle, other.m_handle);
}

void thread::join()
{
    static_cast<thread_data*>(m_handle)->join();
}

void thread::cancel()
{
    static_cast<thread_data*>(m_handle)->cancel();
}

void thread::test_cancel()
{
    thread self;
    static_cast<thread_data*>(self.m_handle)->test_cancel();
}

void thread::set_scheduling_parameter(int policy, const sched_param& param)
{
    static_cast<thread_data*>(m_handle)->set_scheduling_parameter(policy,
        param);
}

void thread::get_scheduling_parameter(int& policy, sched_param& param) const
{
    static_cast<thread_data*>(m_handle)->get_scheduling_parameter(policy,
        param);
}

int thread::max_priority(int policy)
{
#if defined(BOOST_HAS_WINTHREADS)
    if (policy != sched_other)
        throw invalid_thread_argument();
    return THREAD_PRIORITY_HIGHEST;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
#   endif
    return 0;
#endif
}

int thread::min_priority(int policy)
{
#if defined(BOOST_HAS_WINTHREADS)
    if (policy != sched_other)
        throw invalid_thread_argument();
    return THREAD_PRIORITY_LOWEST;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
#   endif
    return 0;
#endif
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
        thread::test_cancel();
        xtime cur;
        xtime_get(&cur, TIME_UTC);
        if (xtime_cmp(xt, cur) <= 0)
            return;
    }
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

thread::id_type thread::id() const
{
    return static_cast<thread_data*>(m_handle)->id();
}

#if defined(BOOST_HAS_WINTHREADS)
const int thread::stack_min = 0;
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(PTHREAD_STACK_MIN)
const int thread::stack_min = PTHREAD_STACK_MIN;
#   else
const int thread::stack_min = 0;
#   endif
#endif

thread_group::thread_group()
{
}

thread_group::~thread_group()
{
}

thread thread_group::create_thread(const function0<void>& threadfunc)
{
    // No scoped_lock required here since the only "shared data" that's
    // modified here occurs inside add_thread which does scoped_lock.
    thread thrd(threadfunc);
    add_thread(thrd);
    return thrd;
}

void thread_group::add_thread(thread thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to add a thread object multiple
    // times. Should we consider this an error and either throw or return an
    // error value?
    std::list<thread>::iterator it = std::find(m_threads.begin(),
        m_threads.end(), thrd);
    assert(it == m_threads.end());
    if (it == m_threads.end())
        m_threads.push_back(thrd);
}

void thread_group::remove_thread(thread thrd)
{
    mutex::scoped_lock scoped_lock(m_mutex);

    // For now we'll simply ignore requests to remove a thread object that's
    // not in the group. Should we consider this an error and either throw or
    // return an error value?
    std::list<thread>::iterator it = std::find(m_threads.begin(),
        m_threads.end(), thrd);

    assert(it != m_threads.end());
    if (it != m_threads.end())
        m_threads.erase(it);
}

void thread_group::join_all()
{
    mutex::scoped_lock scoped_lock(m_mutex);
    for (std::list<thread>::iterator it = m_threads.begin();
         it != m_threads.end(); ++it)
    {
        it->join();
    }
}

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//    1 Jun 01  WEKEMPF Added boost::thread initial implementation.
//    3 Jul 01  WEKEMPF Redesigned boost::thread to be noncopyable.
