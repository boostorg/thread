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

#include <utils.inl>

namespace {

int test_value;

void simple_thread()
{
    test_value = 999;
}

void test_create_thread()
{
    test_value = 0;
    boost::thread_group thrds;
    BOOST_TEST(thrds.create_thread(&simple_thread) != 0);
    thrds.join_all();
    BOOST_TEST(test_value == 999);
}

void test_add_find_remove()
{
    test_value = 0;
    boost::thread_group thrds;
    boost::thread* pthread = new boost::thread(&simple_thread);
    thrds.add_thread(pthread);
    BOOST_TEST(thrds.find(*pthread) == pthread);
    thrds.remove_thread(pthread);
    BOOST_TEST(thrds.find(*pthread) == 0);
    pthread->join();
    BOOST_TEST(test_value == 999);
}

} // namespace

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread_group test suite");

    test->add(BOOST_TEST_CASE(&test_create_thread));
    test->add(BOOST_TEST_CASE(&test_add_find_remove));

    return test;
}
