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

#include <boost/thread/detail/config.hpp>

#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/test/unit_test.hpp>

#include <libs/thread/test/util.inl>

int once_value = 0;
boost::once_flag once = BOOST_ONCE_INIT;
boost::mutex mutex;

void init_once_value()
{
    boost::mutex::scoped_lock lock(mutex);
    once_value++;
    BOOST_CHECK_EQUAL(once_value, 1);
}

void test_once_thread()
{
    unsigned const loop_count=100;
    for(unsigned i=0;i<loop_count;++i)
    {
        boost::call_once(init_once_value, once);
        BOOST_CHECK_EQUAL(once_value, 1);
    }
}

void do_test_once()
{
    const int NUMTHREADS=100;
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
