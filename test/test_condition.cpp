// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>

#include <libs/thread/test/util.inl>

struct condition_test_data
{
    condition_test_data() : notified(0), awoken(0) { }

    boost::mutex mutex;
    boost::condition_variable condition;
    int notified;
    int awoken;
};

void condition_test_thread(condition_test_data* data)
{
    boost::mutex::scoped_lock lock(data->mutex);
    BOOST_CHECK(lock ? true : false);
    while (!(data->notified > 0))
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    data->awoken++;
}

struct cond_predicate
{
    cond_predicate(int& var, int val) : _var(var), _val(val) { }

    bool operator()() { return _var == _val; }

    int& _var;
    int _val;
};

void condition_test_waits(condition_test_data* data)
{
    boost::mutex::scoped_lock lock(data->mutex);
    BOOST_CHECK(lock ? true : false);

    // Test wait.
    while (data->notified != 1)
        data->condition.wait(lock);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test timed_wait.
    boost::xtime xt = delay(10);
    while (data->notified != 3)
        data->condition.timed_wait(lock, xt);
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK_EQUAL(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait.
    xt = delay(10);
    cond_predicate pred(data->notified, 4);
    BOOST_CHECK(data->condition.timed_wait(lock, xt, pred));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred());
    BOOST_CHECK_EQUAL(data->notified, 4);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate timed_wait with relative timeout
    cond_predicate pred_rel(data->notified, 5);
    BOOST_CHECK(data->condition.timed_wait(lock, boost::posix_time::seconds(10), pred_rel));
    BOOST_CHECK(lock ? true : false);
    BOOST_CHECK(pred_rel());
    BOOST_CHECK_EQUAL(data->notified, 5);
    data->awoken++;
    data->condition.notify_one();
}

void do_test_condition_notify_one()
{
    condition_test_data data;

    boost::thread thread(bind(&condition_test_thread, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_CHECK(lock ? true : false);
        data.notified++;
        data.condition.notify_one();
    }

    thread.join();
    BOOST_CHECK_EQUAL(data.awoken, 1);
}

void test_condition_notify_one()
{
    timed_test(&do_test_condition_notify_one, 100, execution_monitor::use_mutex);
}

void do_test_condition_notify_all()
{
    const int NUMTHREADS = 5;
    boost::thread_group threads;
    condition_test_data data;

    try
    {
        for (int i = 0; i < NUMTHREADS; ++i)
            threads.create_thread(bind(&condition_test_thread, &data));

        {
            boost::mutex::scoped_lock lock(data.mutex);
            BOOST_CHECK(lock ? true : false);
            data.notified++;
            data.condition.notify_all();
        }

        threads.join_all();
    }
    catch(...)
    {
        threads.interrupt_all();
        threads.join_all();
        throw;
    }

    BOOST_CHECK_EQUAL(data.awoken, NUMTHREADS);
}

void test_condition_notify_all()
{
    // We should have already tested notify_one here, so
    // a timed test with the default execution_monitor::use_condition
    // should be OK, and gives the fastest performance
    timed_test(&do_test_condition_notify_all, 100);
}

void do_test_condition_waits()
{
    condition_test_data data;

    boost::thread thread(bind(&condition_test_waits, &data));

    {
        boost::mutex::scoped_lock lock(data.mutex);
        BOOST_CHECK(lock ? true : false);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 1);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 2);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 3);

        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 4)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 4);


        boost::thread::sleep(delay(1));
        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 5)
            data.condition.wait(lock);
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK_EQUAL(data.awoken, 5);
    }

    thread.join();
    BOOST_CHECK_EQUAL(data.awoken, 5);
}

void test_condition_waits()
{
    // We should have already tested notify_one here, so
    // a timed test with the default execution_monitor::use_condition
    // should be OK, and gives the fastest performance
    timed_test(&do_test_condition_waits, 12);
}

void do_test_condition_wait_is_a_interruption_point()
{
    condition_test_data data;

    boost::thread thread(bind(&condition_test_thread, &data));

    thread.interrupt();
    thread.join();
    BOOST_CHECK_EQUAL(data.awoken,0);
}


void test_condition_wait_is_a_interruption_point()
{
    timed_test(&do_test_condition_wait_is_a_interruption_point, 1);
}

bool fake_predicate()
{
    return false;
}


void do_test_timed_wait_times_out()
{
    boost::condition_variable cond;
    boost::mutex m;

    boost::posix_time::seconds const delay(5);
    boost::mutex::scoped_lock lock(m);
    boost::system_time const start=boost::get_system_time();
    bool const res=cond.timed_wait(lock,delay,fake_predicate);
    boost::system_time const end=boost::get_system_time();
    BOOST_CHECK(!res);
    BOOST_CHECK((delay-boost::posix_time::milliseconds(10))<=(end-start));
}


void test_timed_wait_times_out()
{
    timed_test(&do_test_timed_wait_times_out, 15);
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: condition test suite");

    test->add(BOOST_TEST_CASE(&test_condition_notify_one));
    test->add(BOOST_TEST_CASE(&test_condition_notify_all));
    test->add(BOOST_TEST_CASE(&test_condition_waits));
    test->add(BOOST_TEST_CASE(&test_condition_wait_is_a_interruption_point));
    test->add(BOOST_TEST_CASE(&test_timed_wait_times_out));

    return test;
}
