// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>

void test_xtime_cmp()
{
    boost::xtime xt1, xt2, cur;
    BOOST_CHECK_EQUAL(
        boost::xtime_get(&cur, boost::TIME_UTC),
        static_cast<int>(boost::TIME_UTC));

    xt1 = xt2 = cur;
    xt1.nsec -= 1;
    xt2.nsec += 1;

    BOOST_CHECK(boost::xtime_cmp(xt1, cur) < 0);
    BOOST_CHECK(boost::xtime_cmp(xt2, cur) > 0);
    BOOST_CHECK(boost::xtime_cmp(cur, cur) == 0);

    xt1 = xt2 = cur;
    xt1.sec -= 1;
    xt2.sec += 1;

    BOOST_CHECK(boost::xtime_cmp(xt1, cur) < 0);
    BOOST_CHECK(boost::xtime_cmp(xt2, cur) > 0);
    BOOST_CHECK(boost::xtime_cmp(cur, cur) == 0);
}

void test_xtime_get()
{
    boost::xtime orig, cur, old;
    BOOST_CHECK_EQUAL(
        boost::xtime_get(&orig,
            boost::TIME_UTC), static_cast<int>(boost::TIME_UTC));
    old = orig;

    for (int x=0; x < 100; ++x)
    {
        BOOST_CHECK_EQUAL(
            boost::xtime_get(&cur, boost::TIME_UTC),
            static_cast<int>(boost::TIME_UTC));
        BOOST_CHECK(boost::xtime_cmp(cur, orig) >= 0);
        BOOST_CHECK(boost::xtime_cmp(cur, old) >= 0);
        old = cur;
    }
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: xtime test suite");

    test->add(BOOST_TEST_CASE(&test_xtime_cmp));
    test->add(BOOST_TEST_CASE(&test_xtime_get));

    return test;
}
