// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>

#include <boost/test/unit_test.hpp>

#include <libs/thread/test/util.inl>

int once_value = 0;
boost::once_flag once = BOOST_ONCE_INIT;

void init_once_value()
{
    once_value++;
}

void test_once_thread()
{
    boost::call_once(init_once_value, once);
}

void do_test_once()
{
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_once_thread);
    threads.join_all();
    BOOST_CHECK_EQUAL(once_value, 1);
}

void test_once()
{
    timed_test(&do_test_once, 2);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: once test suite");

    test->add(BOOST_TEST_CASE(test_once));

    return test;
}
