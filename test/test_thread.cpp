#include <list>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/tss.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

//#define BOOST_INCLUDE_MAIN
//#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

template <typename M>
void test_lock(M* dummy=0)
{
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        lock_type lock(mutex, false);
        BOOST_CHECK(!lock);
    }
    lock_type lock(mutex);
    BOOST_CHECK(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_CHECK(!condition.timed_wait(lock, xt));
    BOOST_CHECK(lock);

    // Test the lock and unlock methods.
    lock.unlock();
    BOOST_CHECK(!lock);
    lock.lock();
    BOOST_CHECK(lock);
}

template <typename M>
void test_trylock(M* dummy=0)
{
    typedef M mutex_type;
    typedef typename M::scoped_try_lock try_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        try_lock_type lock(mutex);
        BOOST_CHECK(lock);
    }
    {
        try_lock_type lock(mutex, false);
        BOOST_CHECK(!lock);
    }
    try_lock_type lock(mutex, true);
    BOOST_CHECK(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_CHECK(!condition.timed_wait(lock, xt));
    BOOST_CHECK(lock);

    // Test the lock, unlock and trylock methods.
    lock.unlock();
    BOOST_CHECK(!lock);
    lock.lock();
    BOOST_CHECK(lock);
    lock.unlock();
    BOOST_CHECK(!lock);
    BOOST_CHECK(lock.try_lock());
    BOOST_CHECK(lock);
}

template <typename M>
void test_timedlock(M* dummy=0)
{
    typedef M mutex_type;
    typedef typename M::scoped_timed_lock timed_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt;
        BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
        xt.nsec += 100000000;

        timed_lock_type lock(mutex, xt);
        BOOST_CHECK(lock);
    }
    {
        timed_lock_type lock(mutex, false);
        BOOST_CHECK(!lock);
    }
    timed_lock_type lock(mutex, true);
    BOOST_CHECK(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_CHECK(!condition.timed_wait(lock, xt));
    BOOST_CHECK(lock);

    // Test the lock, unlock and timedlock methods.
    lock.unlock();
    BOOST_CHECK(!lock);
    lock.lock();
    BOOST_CHECK(lock);
    lock.unlock();
    BOOST_CHECK(!lock);
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.nsec += 100000000;
    BOOST_CHECK(lock.timed_lock(xt));
}

void test_mutex()
{
    typedef boost::mutex mutex;
    test_lock<mutex>();
}

void test_try_mutex()
{
    typedef boost::try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
}

void test_timed_mutex()
{
    typedef boost::timed_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    test_timedlock<mutex>();
}

void test_recursive_mutex()
{
    typedef boost::recursive_mutex mutex;
    test_lock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_try_mutex()
{
    typedef boost::recursive_try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_timed_mutex()
{
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
    BOOST_CHECK(lock);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    BOOST_CHECK(lock);
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
    condition_test_data data;

    boost::thread thread(thread_adapter(&condition_test_thread, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_CHECK(lock);
        data.notified++;
        data.condition.notify_one();
    }

    thread.join();
    BOOST_CHECK_EQUAL(data.awoken, 1);
}

void test_condition_notify_all()
{
    const int NUMTHREADS = 5;
    boost::thread_group threads;
    condition_test_data data;

    for (int i = 0; i < NUMTHREADS; ++i)
        threads.create_thread(thread_adapter(&condition_test_thread, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_CHECK(lock);
        data.notified++;
        data.condition.notify_all();
    }

    threads.join_all();
    BOOST_CHECK_EQUAL(data.awoken, NUMTHREADS);
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
    BOOST_CHECK(lock);

    // Test wait.
    while (data->notified != 1)
        data->condition.wait(lock);
    BOOST_CHECK(lock);
    BOOST_CHECK_EQUAL(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_CHECK(lock);
    BOOST_CHECK_EQUAL(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.nsec += 100000000;
    while (data->notified != 3)
        data->condition.timed_wait(lock, xt);
    BOOST_CHECK(lock);
    BOOST_CHECK_EQUAL(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.sec += 2;
    BOOST_CHECK(data->condition.timed_wait(lock, xt, cond_predicate(data->notified, 4)));
    BOOST_CHECK(lock);
    BOOST_CHECK_EQUAL(data->notified, 4);
    data->awoken++;
}

void test_condition_waits()
{
    condition_test_data data;

    boost::thread thread(thread_adapter(&condition_test_waits, &data));

    boost::xtime xt;

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_CHECK(lock);

        BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        BOOST_CHECK_EQUAL(data.awoken, 1);

        BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        BOOST_CHECK_EQUAL(data.awoken, 2);

        BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
        xt.sec += 1;
        boost::thread::sleep(xt);
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        BOOST_CHECK_EQUAL(data.awoken, 3);
    }

    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.sec += 1;
    boost::thread::sleep(xt);
    data.notified++;
    data.condition.notify_one();
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
    xt.sec += 1;
    boost::thread::sleep(xt);
    thread.join();
    BOOST_CHECK_EQUAL(data.awoken, 4);
}

void test_condition()
{
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
        BOOST_CHECK_EQUAL(n, i);
        ++n;
    }
}

void test_tss()
{
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_tss_thread);
    threads.join_all();
    BOOST_CHECK_EQUAL(tss_instances, 0);
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
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_once_thread);
    threads.join_all();
    BOOST_CHECK_EQUAL(once_value, 1);
}

//int test_main(int, char*[])
boost::unit_test_framework::test_suite* init_unit_test_suite(int argc, char* argv[])
{
	boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads test suite");

    test->add(BOOST_TEST_CASE(&test_mutex));
    test->add(BOOST_TEST_CASE(&test_try_mutex));
    test->add(BOOST_TEST_CASE(&test_timed_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_try_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_timed_mutex));
    test->add(BOOST_TEST_CASE(&test_condition));
    test->add(BOOST_TEST_CASE(&test_tss));
    test->add(BOOST_TEST_CASE(&test_once));

    return test;
}
