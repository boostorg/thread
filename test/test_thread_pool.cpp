#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/test/test_tools.hpp>

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

void chatty_worker()
{
    work_counts[CHATTY_WORKER]++;
}

void fast_worker()
{
    work_counts[FAST_WORKER]++;
}

void slow_worker()
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC);
    
    xt.sec++;

    boost::thread::sleep(xt);

    work_counts[SLOW_WORKER]++;
}

void cpubound_worker()
{
    double d;
    double limit = SQRT_PER_SECOND/2.0;
    for(d = 1.0; d < limit; d+=1.0)
    {
        sqrt(d);
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
    slow_worker();
}

// Test a thread_pool with all different sorts of workers
void test_heterogeneous()
{
	memset(work_counts,0,sizeof(work_counts));

    boost::thread_pool tp(MAX_POOL_THREADS,MIN_POOL_THREADS,POOL_TIMEOUT);

    for(int i = 0; i < ITERATIONS; i++)
    {
        tp.add(&chatty_worker);
        tp.add(&fast_worker);
        tp.add(&slow_worker);
        tp.add(&cpubound_worker);
    }

    tp.join();

    BOOST_TEST(work_counts[CHATTY_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[FAST_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[SLOW_WORKER] == ITERATIONS);
    BOOST_TEST(work_counts[CPUBOUND_WORKER] == ITERATIONS);
}

void test_recursive()
{
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
	int wc_after_cancel[WORKER_TYPE_COUNT];
    
    memset(work_counts,0,sizeof(work_counts));

    boost::thread_pool tp(MAX_POOL_THREADS,MIN_POOL_THREADS,POOL_TIMEOUT);

    for(int i = 0; i < ITERATIONS; i++)
    {
        tp.add(&chatty_worker);
        tp.add(&fast_worker);
        tp.add(&slow_worker);
        tp.add(&cpubound_worker);
    }

    tp.cancel();

    // Save our worker counts
    memcpy(wc_after_cancel,work_counts,sizeof(wc_after_cancel));

    // Do a bit more work to prove we can continue after a cancel
    tp.add(&chatty_worker);
    tp.add(&fast_worker);
    tp.add(&slow_worker);
    tp.add(&cpubound_worker);

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

