// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>

#define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_sleep_only
#include <libs/thread/test/util.inl>

int test_value;

void simple_thread()
{
    test_value = 999;
}

void comparison_thread(boost::thread* parent)
{
    boost::thread thrd;
    BOOST_CHECK(thrd != *parent);
    boost::thread thrd2;
    BOOST_CHECK(thrd == thrd2);
}

void test_sleep()
{
    boost::xtime xt = delay(3);
    boost::thread::sleep(xt);

    // Ensure it's in a range instead of checking actual equality due to time
    // lapse
    BOOST_CHECK(in_range(xt, 2));
}

void do_test_creation()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
    thrd.join();
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_creation()
{
    timed_test(&do_test_creation, 1);
}

void do_test_comparison()
{
    boost::thread self;
    boost::thread thrd(bind(&comparison_thread, &self));
    thrd.join();
}

void test_comparison()
{
    timed_test(&do_test_comparison, 1);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_comparison));

    return test;
}
