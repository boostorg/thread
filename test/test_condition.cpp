#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/test/test_tools.hpp>

namespace {

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

} // namespace

void test_condition()
{
    test_condition_notify_one();
    test_condition_notify_all();
    test_condition_waits();
}
