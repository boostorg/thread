#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/thread_pool.hpp>

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

#include <list>
#include <iostream>

template <typename M>
void test_lock(M* dummy=0)
{
	// Indicate testing progress...
	std::cout << '.';

    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    lock_type lock(mutex);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock and unlock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
}

template <typename M>
void test_trylock(M* dummy=0)
{
	// Indicate testing progress...
	std::cout << '.';

	typedef M mutex_type;
    typedef typename M::scoped_try_lock try_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        try_lock_type lock(mutex);
        BOOST_TEST(lock);
    }
    {
        try_lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    try_lock_type lock(mutex, true);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock, unlock and trylock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
    lock.unlock();
    BOOST_TEST(!lock);
    BOOST_TEST(lock.try_lock());
    BOOST_TEST(lock);
}

template <typename M>
void test_timedlock(M* dummy=0)
{
	// Indicate testing progress...
	std::cout << '.';

    typedef M mutex_type;
    typedef typename M::scoped_timed_lock timed_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt;
        BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
        xt.nsec += 100000000;

        timed_lock_type lock(mutex, xt);
        BOOST_TEST(lock);
    }
    {
        timed_lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    timed_lock_type lock(mutex, true);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock, unlock and timedlock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
    lock.unlock();
    BOOST_TEST(!lock);
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;
    BOOST_TEST(lock.timed_lock(xt));
}

void test_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::mutex mutex;
    test_lock<mutex>();
}

void test_try_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
}

void test_timed_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::timed_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    test_timedlock<mutex>();
}

void test_recursive_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::recursive_mutex mutex;
    test_lock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_try_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::recursive_try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_timed_mutex()
{
	// Indicate testing progress...
	std::cout << '.';

    typedef boost::recursive_timed_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    test_timedlock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

struct condition_test_data
{
    condition_test_data() : notified(0), awoken(0) { }

    boost::mutex mutex;
    boost::condition condition;
    int notified;
    int awoken;
};

void condition_test_thread(void* param)
{
    condition_test_data* data = static_cast<condition_test_data*>(param);
    boost::mutex::scoped_lock lock(data->mutex);
    BOOST_TEST(lock);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    BOOST_TEST(lock);
    data->awoken++;
}

class thread_adapter
{
public:
    thread_adapter(void (*func)(void*), void* param) : _func(func), _param(param) { }
    void operator()() const { _func(_param); }
private:
    void (*_func)(void*);
    void* _param;
};

void test_condition_notify_one()
{
	// Indicate testing progress...
	std::cout << '.';

    condition_test_data data;

    boost::thread thread(thread_adapter(&condition_test_thread, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_TEST(lock);
        data.notified++;
        data.condition.notify_one();
    }

    thread.join();
    BOOST_TEST(data.awoken == 1);
}

void test_condition_notify_all()
{
	// Indicate testing progress...
	std::cout << '.';

    const int NUMTHREADS = 5;
    boost::thread_group threads;
    condition_test_data data;

    for (int i = 0; i < NUMTHREADS; ++i)
        threads.create_thread(thread_adapter(&condition_test_thread, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_TEST(lock);
        data.notified++;
        data.condition.notify_all();
    }

    threads.join_all();
    BOOST_TEST(data.awoken == NUMTHREADS);
}

struct cond_predicate
{
    cond_predicate(int& var, int val) : _var(var), _val(val) { }

    bool operator()() { return _var == _val; }

    int& _var;
    int _val;
};

void condition_test_waits(void* param)
{
    condition_test_data* data = static_cast<condition_test_data*>(param);

    boost::mutex::scoped_lock lock(data->mutex);
    BOOST_TEST(lock);

    // Test wait.
    while (data->notified != 1)
        data->condition.wait(lock);
    BOOST_TEST(lock);
    BOOST_TEST(data->notified == 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_TEST(lock);
    BOOST_TEST(data->notified == 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;
    while (data->notified != 3)
        data->condition.timed_wait(lock, xt);
    BOOST_TEST(lock);
    BOOST_TEST(data->notified == 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.sec += 2;
    BOOST_TEST(data->condition.timed_wait(lock, xt, cond_predicate(data->notified, 4)));
    BOOST_TEST(lock);
    BOOST_TEST(data->notified == 4);
    data->awoken++;
}

void test_condition_waits()
{
	// Indicate testing progress...
	std::cout << '.';

    condition_test_data data;

    boost::thread thread(thread_adapter(&condition_test_waits, &data));

    boost::xtime xt;

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_TEST(lock);

        BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        BOOST_TEST(data.awoken == 1);

        BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        BOOST_TEST(data.awoken == 2);

        BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        BOOST_TEST(data.awoken == 3);
    }

    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.sec += 1;
    boost::thread::sleep(xt);
    data.notified++;
    data.condition.notify_one();
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.sec += 1;
    boost::thread::sleep(xt);
    thread.join();
    BOOST_TEST(data.awoken == 4);
}

void test_condition()
{
	// Indicate testing progress...
	std::cout << '.';

    test_condition_notify_one();
    test_condition_notify_all();
    test_condition_waits();
}

boost::mutex tss_mutex;
int tss_instances = 0;

struct tss_value_t
{
    tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        ++tss_instances;
        value = 0;
    }
    ~tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        --tss_instances;
    }
    int value;
};

boost::thread_specific_ptr<tss_value_t> tss_value;

void test_tss_thread()
{
    tss_value.reset(new tss_value_t());
    for (int i=0; i<1000; ++i)
    {
        int& n = tss_value->value;
        BOOST_TEST(n == i);
        ++n;
    }
}

void test_tss()
{
	// Indicate testing progress...
	std::cout << '.';

    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_tss_thread);
    threads.join_all();
    BOOST_TEST(tss_instances == 0);
}

int once_value = 0;
boost::once_flag once = BOOST_ONCE_INIT;

void init_once_value()
{
    once_value++;
}

void test_once_thread()
{
    boost::call_once(&init_once_value, once);
}

void test_once()
{
	// Indicate testing progress...
	std::cout << '.';

    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_once_thread);
    threads.join_all();
    BOOST_TEST(once_value == 1);
}

// Shared variables for generation barrier test
const int N_THREADS=10;
boost::barrier gen_barrier(N_THREADS);
boost::mutex mutex;
long global_parameter;

void barrier_thread()
{
    for (int i = 0; i < 5; ++i)
    {
        if (gen_barrier.wait())
		{
			boost::mutex::scoped_lock lock(mutex);
            global_parameter++;
		}
    }
}

void test_barrier()
{
	// Indicate testing progress...
	std::cout << '.';

    boost::thread_group g;
    global_parameter = 0;

    for (int i = 0; i < N_THREADS; ++i)
        g.create_thread(&barrier_thread);

    g.join_all();

    BOOST_TEST(global_parameter == 5);
}

const int MAX_POOL_THREADS=8;
const int MIN_POOL_THREADS=2;
const int POOL_TIMEOUT = 2;     // seconds
const int ITERATIONS=25;
boost::mutex     detach_prot;
boost::condition detached;
boost::condition waiting_for_detach;
int at_detach=0;
bool pool_detached=false;
const int DETACH_THREADS=2;

// Constant to cause the cpubound thread to take approx 0.5 seconds
//   to complete.  Doesn't have to be exact, but should take "a while"
const double SQRT_PER_SECOND=3000000.0;

enum
{
    CHATTY_WORKER,
    FAST_WORKER,
    SLOW_WORKER,
    CPUBOUND_WORKER,
    
    WORKER_TYPE_COUNT
};

int work_counts[WORKER_TYPE_COUNT];

class job_adapter
{
public:
    job_adapter(void (*func)(void*), void* param) 
        : _func(func), _param(param){ }
    void operator()() const { _func(_param); }
private:
        void (*_func)(void*);
        void* _param;
};

void chatty_worker(void *arg)
{
    int id = reinterpret_cast<int>(arg);
    work_counts[CHATTY_WORKER]++;
}

void fast_worker(void *)
{
    work_counts[FAST_WORKER]++;
}

void slow_worker(void *)
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC);
    
    xt.sec++;

    boost::thread::sleep(xt);

    work_counts[SLOW_WORKER]++;
}

void cpubound_worker(void *)
{
    double d;
    double limit = SQRT_PER_SECOND/2.0;
    for(d = 1.0; d < limit; d+=1.0)
    {
        double root = sqrt(d);
    }

    work_counts[CPUBOUND_WORKER]++;
}

struct recursive_args
{
    boost::thread_pool *ptp;
    int                count;
};

void recursive_worker(void *arg)
{
    recursive_args *pargs = static_cast<recursive_args *>(arg);
    
    if(--pargs->count > 0)
        pargs->ptp->add(job_adapter(recursive_worker,pargs));
}

void detach_worker(void *arg)
{
    int detach_threads = reinterpret_cast<int>(arg);
    boost::mutex::scoped_lock l(detach_prot);
    
    // If we are the Nth thread to reach this, notify
    //   our caller that everyone is ready to detach!
    if(++at_detach==detach_threads)
        waiting_for_detach.notify_all();

    while(!pool_detached)
        detached.wait(l);

    // Call slow worker to do a bit of work after this...
    slow_worker(arg);
}

// Test a thread_pool with all different sorts of workers
void test_heterogeneous()
{
	// Indicate testing progress...
	std::cout << '.';

	memset(work_counts,0,sizeof(work_counts));

    boost::thread_pool tp(MAX_POOL_THREADS,MIN_POOL_THREADS,POOL_TIMEOUT);

    for(int i = 0; i < ITERATIONS; i++)
    {
        tp.add(job_adapter(chatty_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(fast_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(slow_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(cpubound_worker,reinterpret_cast<void *>(i)));
    }

    tp.join();

    BOOST_TEST(work_counts[CHATTY_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[FAST_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[SLOW_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[CPUBOUND_WORKER] == ITERATIONS);
}

void test_recursive()
{
	// Indicate testing progress...
	std::cout << '.';

	recursive_args ra;

    boost::thread_pool tp;
    ra.ptp = &tp;
    ra.count = ITERATIONS;

    // Recursive_worker will add another job to the queue before returning
    tp.add(job_adapter(recursive_worker,static_cast<void *>(&ra)));

    // busy wait for bottom to be reached.
    while(ra.count > 0)
        boost::thread::yield();
    
    tp.join();

    BOOST_TEST(ra.count == 0);
}

// Test cancellation of thread_pool operations.

void test_cancel()
{
	// Indicate testing progress...
	std::cout << '.';

	int wc_after_cancel[WORKER_TYPE_COUNT];
    
    memset(work_counts,0,sizeof(work_counts));

    boost::thread_pool tp(MAX_POOL_THREADS,MIN_POOL_THREADS,POOL_TIMEOUT);

    for(int i = 0; i < ITERATIONS; i++)
    {
        tp.add(job_adapter(chatty_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(fast_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(slow_worker,reinterpret_cast<void *>(i)));
        tp.add(job_adapter(cpubound_worker,reinterpret_cast<void *>(i)));
    }

    tp.cancel();

    // Save our worker counts
    memcpy(wc_after_cancel,work_counts,sizeof(wc_after_cancel));

    // Do a bit more work to prove we can continue after a cancel
    tp.add(job_adapter(chatty_worker,reinterpret_cast<void *>(i)));
    tp.add(job_adapter(fast_worker,reinterpret_cast<void *>(i)));
    tp.add(job_adapter(slow_worker,reinterpret_cast<void *>(i)));
    tp.add(job_adapter(cpubound_worker,reinterpret_cast<void *>(i)));

    tp.join();

    // Check our counts

    // As long as ITERATIONS is decently sized, there is no way
    //   these tasks could have completed before the cancel...
    BOOST_TEST(wc_after_cancel[SLOW_WORKER] < ITERATIONS);
    BOOST_TEST(wc_after_cancel[CPUBOUND_WORKER] < ITERATIONS);

    // Since they could not have completed, if we are processing jobs
    //   in a FIFO order, the others can't have completed either.
    BOOST_TEST(wc_after_cancel[CHATTY_WORKER] < ITERATIONS);
    BOOST_TEST(wc_after_cancel[FAST_WORKER] < ITERATIONS);


    // Check to see that more work was accomplished after the cancel.
    BOOST_TEST(wc_after_cancel[SLOW_WORKER] < work_counts[SLOW_WORKER]);
    BOOST_TEST(wc_after_cancel[CPUBOUND_WORKER] < work_counts[CPUBOUND_WORKER]);
    BOOST_TEST(wc_after_cancel[CHATTY_WORKER] < work_counts[CHATTY_WORKER]);
    BOOST_TEST(wc_after_cancel[FAST_WORKER] < work_counts[FAST_WORKER]);
}

void test_detach()
{
	// Indicate testing progress...
	std::cout << '.';

	int wc_after_detach;

    memset(work_counts,0,sizeof(work_counts));


    {
        boost::mutex::scoped_lock l(detach_prot);

        // For detach testing, we want a known size thread pool so that we can make a better guess
        //   at when the detached process will finish
        boost::thread_pool tp(DETACH_THREADS,0);

        for(int i = 0; i < DETACH_THREADS; i++)
        {
            tp.add(job_adapter(detach_worker,reinterpret_cast<void *>(DETACH_THREADS)));
        }

        // Wait for all of the threads to reach a known point
        waiting_for_detach.wait(l);

        tp.detach();

        wc_after_detach = work_counts[SLOW_WORKER];

        // Let our threads know we've detached.
        pool_detached = true;
        detached.notify_all();
    }

    // Our detached threads should finish approx 1 sec after this.
    //   We could reliably sync. with the exit of detach_worker, but we
    //   can't reliably sync. with the cleanup of the thread_pool harness,
    //   so for the purposes of this test, we'll sleep 3 secs, and check some values.

    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC);
    xt.sec += 3;
    boost::thread::sleep(xt);

    // Work should still complete after detach
    BOOST_TEST(work_counts[SLOW_WORKER] == DETACH_THREADS);
    // None of the work should have occurred before attach.
    BOOST_TEST(0 ==  wc_after_detach);
}

void test_thread_pool()
{
    test_heterogeneous();
    test_recursive();
    test_cancel();
    test_detach();
}

int test_main(int, char*[])
{
    test_mutex();
    test_try_mutex();
    test_timed_mutex();
    test_recursive_mutex();
    test_recursive_try_mutex();
    test_recursive_timed_mutex();
    test_condition();
    test_tss();
    test_once();
	test_barrier();
	test_thread_pool();
    return 0;
}
