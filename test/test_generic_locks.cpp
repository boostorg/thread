// (C) Copyright 2008 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>

void test_lock_two_uncontended()
{
    boost::mutex m1,m2;

    boost::mutex::scoped_lock l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
    
    boost::lock(l1,l2);
    
    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: generic locks test suite");

    test->add(BOOST_TEST_CASE(&test_lock_two_uncontended));

    return test;
}
