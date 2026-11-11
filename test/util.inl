// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007-8 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(UTIL_INL_WEK01242003)
#define UTIL_INL_WEK01242003

#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/config.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/atomic.hpp>

#ifndef DEFAULT_EXECUTION_MONITOR_TYPE
#   define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_condition
#endif

// boostinspect:nounnamed



namespace
{
inline boost::xtime delay(int secs, int msecs=0, int nsecs=0)
{
    const int MILLISECONDS_PER_SECOND = 1000;
    const int NANOSECONDS_PER_SECOND = 1000000000;
    const int NANOSECONDS_PER_MILLISECOND = 1000000;

    boost::xtime xt;
    if (boost::TIME_UTC_ != boost::xtime_get (&xt, boost::TIME_UTC_))
        BOOST_ERROR("boost::xtime_get != boost::TIME_UTC_");

    nsecs += xt.nsec;
    msecs += nsecs / NANOSECONDS_PER_MILLISECOND;
    secs += msecs / MILLISECONDS_PER_SECOND;
    nsecs += (msecs % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
    xt.nsec = nsecs % NANOSECONDS_PER_SECOND;
    xt.sec += secs + (nsecs / NANOSECONDS_PER_SECOND);

    return xt;
}

}
namespace boost
{
namespace threads
{
namespace test
{
inline bool in_range(const boost::xtime& xt, int secs=1)
{
    boost::xtime min = delay(-secs);
    boost::xtime max = delay(0);
    return (boost::xtime_cmp(xt, min) >= 0) &&
        (boost::xtime_cmp(xt, max) <= 0);
}
}
}
}


namespace
{
class execution_monitor
{
private:
    enum execution_state { initial, started, finished };

public:
    enum wait_type { use_sleep_only, use_mutex, use_condition };

    execution_monitor(wait_type type, int secs)
        : state(initial), type(type), secs(secs) { }
    void start()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        state.store(started, boost::memory_order_relaxed);
        cond.notify_one();
    }
    void finish()
    {
        if (type != use_sleep_only) {
            boost::unique_lock<boost::mutex> lock(mutex);
            state.store(finished, boost::memory_order_release);
            if (type == use_condition)
                cond.notify_one();
        } else {
            state.store(finished, boost::memory_order_release);
        }
    }
    bool wait()
    {
        if (state.load(boost::memory_order_relaxed) == initial) {
            boost::unique_lock<boost::mutex> lock(mutex);
            while (state.load(boost::memory_order_relaxed) == initial) {
                cond.wait(lock);
            }
        }

        boost::xtime xt = delay(secs);
        if (type == use_sleep_only) {
            boost::thread::sleep(xt);
            return state.load(boost::memory_order_acquire) == finished;
        }
        if (type == use_condition) {
            boost::unique_lock<boost::mutex> lock(mutex);
            while (state.load(boost::memory_order_relaxed) != finished) {
                if (!cond.timed_wait(lock, xt))
                    break;
            }
            return state.load(boost::memory_order_acquire) == finished;
        }
        for (int i = 0; ; ++i) {
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                if (state.load(boost::memory_order_acquire) == finished)
                    return true;
                else if (i > secs * 2)
                    return state.load(boost::memory_order_acquire) == finished;
            }
            boost::thread::sleep(delay(0, 500));
        }
    }

private:
    boost::mutex mutex;
    boost::condition cond;
    boost::atomic<execution_state> state;
    wait_type type;
    int secs;
};
}
namespace thread_detail_anon
{
template <typename F>
class indirect_adapter
{
public:
    indirect_adapter(F func, execution_monitor& monitor)
        : func(func), monitor(monitor) { }
#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
    indirect_adapter(indirect_adapter const&) = default;
#endif

    void operator()() const
    {
        monitor.start();
        try
        {
            func();
        }
        catch (...)
        {
            monitor.finish();
            throw;
        }
        monitor.finish();
    }

private:
    F func;
    execution_monitor& monitor;
    void operator=(indirect_adapter&);
};

}
// boostinspect:nounnamed
namespace 
{

template <typename F>
void timed_test(F func, int secs,
    execution_monitor::wait_type type=DEFAULT_EXECUTION_MONITOR_TYPE)
{
    execution_monitor monitor(type, secs);
    thread_detail_anon::indirect_adapter<F> ifunc(func, monitor);
    boost::thread thrd(ifunc);
    BOOST_REQUIRE_MESSAGE(monitor.wait(),
        "Timed test didn't complete in time, possible deadlock.");
}

}

namespace thread_detail_anon
{

template <typename F, typename T>
class thread_binder
{
public:
    thread_binder(const F& func, const T& param)
        : func(func), param(param) { }
    void operator()() const { func(param); }

private:
    F func;
    T param;
};

}

// boostinspect:nounnamed
namespace 
{
template <typename F, typename T>
thread_detail_anon::thread_binder<F, T> bind(const F& func, const T& param)
{
    return thread_detail_anon::thread_binder<F, T>(func, param);
}
}

namespace thread_detail_anon
{

template <typename R, typename T>
class thread_member_binder
{
public:
    thread_member_binder(R (T::*func)(), T& param)
        : func(func), param(param) { }
#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS)
    thread_member_binder(thread_member_binder const&) = default;
#endif

    void operator()() const { (param.*func)(); }

private:
    void operator=(thread_member_binder&);

    R (T::*func)();
    T& param;
};

}

// boostinspect:nounnamed
namespace 
{
template <typename R, typename T>
thread_detail_anon::thread_member_binder<R, T> bind(R (T::*func)(), T& param)
{
    return thread_detail_anon::thread_member_binder<R, T>(func, param);
}
} // namespace

#endif
