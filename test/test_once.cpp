// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/once.hpp>

boost::once_flag flag=BOOST_ONCE_INIT;
int var_to_init=0;
boost::mutex m;

void initialize_variable()
{
    // ensure that if multiple threads get in here, they are serialized, so we can see the effect
    boost::mutex::scoped_lock lock(m);
    ++var_to_init;
}

void call_once_thread()
{
    unsigned const loop_count=100;
    int my_once_value=0;
    for(unsigned i=0;i<loop_count;++i)
    {
        boost::call_once(flag, initialize_variable);
        my_once_value=var_to_init;
        if(my_once_value!=1)
        {
            break;
        }
    }
    boost::mutex::scoped_lock lock(m);
    BOOST_CHECK_EQUAL(my_once_value, 1);
}

void test_call_once()
{
    unsigned const num_threads=100;
    boost::thread_group group;
    
    for(unsigned i=0;i<num_threads;++i)
    {
        group.create_thread(&call_once_thread);
    }
    group.join_all();
    BOOST_CHECK_EQUAL(var_to_init,1);
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: call_once test suite");

    test->add(BOOST_TEST_CASE(test_call_once));

    return test;
}
