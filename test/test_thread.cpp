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

void comparison_thread(boost::thread::id parent)
{
    boost::thread::id const my_id=boost::this_thread::get_id();
    
    BOOST_CHECK(my_id != parent);
    boost::thread::id const my_id2=boost::this_thread::get_id();
    BOOST_CHECK(my_id == my_id2);

    boost::thread::id const no_thread_id=boost::thread::id();
    BOOST_CHECK(my_id != no_thread_id);
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

void do_test_id_comparison()
{
    boost::thread::id const self=boost::this_thread::get_id();
    boost::thread thrd(bind(&comparison_thread, self));
    thrd.join();
}

void test_id_comparison()
{
    timed_test(&do_test_id_comparison, 1);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_id_comparison));

    return test;
}
