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

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>

#define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_sleep_only
#include "util.inl"

int test_value;

void simple_thread()
{
    std::cout << "simple_thread()";
    test_value = 999;
}

void cancel_thread()
{
    // This block will test the cancellation guard. If it
    // doesn't work, we'll be cancelled with out setting
    // the test_value to 999.
    try
    {
        boost::cancellation_guard guard;
        // Sleep long enough to let the main thread cancel us
        boost::thread::sleep(delay(3));
    }
    catch (boost::thread_cancel& cancel)
    {
        test_value = 666; // indicates unexpected cancellation
        throw; // Make sure to re-throw!
    }

    // This block tests the cancellation itself.  If it
    // works a thread_cancel exception will be thrown,
    // and in the catch handler for it we'll set our
    // exptected test_value of 999.
    try
    {
        boost::thread::test_cancel();
    }
    catch (boost::thread_cancel& cancel)
    {
        test_value = 999;
        throw; // Make sure to re-throw!
    }
}

void priority_thread()
{
    int policy;
    boost::sched_param param;
    boost::thread self;
    self.get_scheduling_parameter(policy, param);
    test_value = param.priority;
    int new_prio = boost::thread::min_priority(policy);
    BOOST_CHECK(new_prio != param.priority);
    param.priority = new_prio;
    self.set_scheduling_parameter(policy, param);
    param.priority++;
    self.get_scheduling_parameter(policy, param);
    BOOST_CHECK_EQUAL(new_prio, param.priority);
}

void comparison_thread(boost::thread* parent)
{
    boost::thread thrd;
    BOOST_TEST(thrd != *parent);
    BOOST_TEST(thrd == boost::thread());
}

void test_sleep()
{
    boost::xtime xt = delay(3);
    boost::thread::sleep(xt);

    // Insure it's in a range instead of checking actual equality due to time
    // lapse
    BOOST_CHECK(in_range(xt));
}

void do_test_creation()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
    boost::thread::sleep(delay(2));
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_creation()
{
    timed_test(&do_test_creation, 3);
}

void do_test_join()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
    thrd.join();
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_join()
{
    timed_test(&do_test_join, 3);
}

void do_test_comparison()
{
    boost::thread self;
    BOOST_CHECK(self == boost::thread());

    boost::thread thrd(bind(&comparison_thread, &self));
    boost::thread thrd2 = thrd;

    BOOST_CHECK(thrd != self);
    BOOST_CHECK(thrd == thrd2);

    thrd.join();
}

void test_comparison()
{
    timed_test(&do_test_comparison, 2);
}

void do_test_cancel()
{
    test_value = 0;
    boost::thread thrd(&cancel_thread);
    thrd.cancel();
    thrd.join();
    BOOST_CHECK_EQUAL(test_value, 999); // only true if thread was cancelled
}

void test_cancel()
{
    timed_test(&do_test_cancel, 4);
}

void test_thread_attributes()
{
    boost::thread::attributes attr;
    try
    {
        size_t size = attr.get_stack_size();
        attr.set_stack_size(size + 1);
        BOOST_CHECK_EQUAL(attr.get_stack_size(), size + 1);
    }
    catch (boost::unsupported_thread_option& err)
    {
    }
    try
    {
        void* stack = attr.get_stack_address();
        attr.set_stack_address((char*)stack + 1);
        BOOST_CHECK_EQUAL(attr.get_stack_address(), (char*)stack + 1);
    }
    catch (boost::unsupported_thread_option& err)
    {
    }
    try
    {
        BOOST_CHECK(attr.inherit_scheduling());
        attr.inherit_scheduling(false);
        BOOST_CHECK(!attr.inherit_scheduling());
        int policy;
        boost::sched_param param;
        attr.get_schedule(policy, param);
        int min = boost::thread::min_priority(policy);
        int max = boost::thread::max_priority(policy);
        int new_prio = min + 1;
        if (new_prio > max) new_prio = max;
        param.priority = new_prio;
        attr.set_schedule(policy, param);
        param.priority++;
        attr.get_schedule(policy, param);
        BOOST_CHECK_EQUAL(new_prio, param.priority);
        int scope = attr.scope();
        if (scope == boost::scope_process)
            scope = boost::scope_system;
        else
            scope = boost::scope_process;
        attr.scope(scope);
        BOOST_CHECK_EQUAL(attr.scope(), scope);
    }
    catch (boost::unsupported_thread_option& err)
    {
    }
}

void test_thread_priority()
{
    boost::thread::attributes attr;
    try
    {
        attr.inherit_scheduling(false);
        BOOST_CHECK(!attr.inherit_scheduling());
        int policy;
        boost::sched_param param;
        attr.get_schedule(policy, param);
        int min = boost::thread::min_priority(policy);
        int max = boost::thread::max_priority(policy);
        int new_prio = min + 1;
        if (new_prio > max) new_prio = max;
        BOOST_CHECK(new_prio != param.priority);
        param.priority = new_prio;
        attr.set_schedule(policy, param);
        test_value = 0;
        boost::thread thrd(&priority_thread, attr);
        thrd.join();
        BOOST_CHECK_EQUAL(test_value, new_prio);
    }
    catch (boost::unsupported_thread_option& err)
    {
    }
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_join));
    test->add(BOOST_TEST_CASE(test_comparison));
    test->add(BOOST_TEST_CASE(test_cancel));
    test->add(BOOST_TEST_CASE(test_thread_attributes));
    test->add(BOOST_TEST_CASE(test_thread_priority));

    return test;
}
