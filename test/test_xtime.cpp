#include <boost/thread/xtime.hpp>

#include <boost/test/unit_test.hpp>

void test_xtime_cmp()
{
	boost::xtime xt1, xt2, cur;
	boost::xtime_get(&cur, boost::TIME_UTC);

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

boost::unit_test_framework::test_suite* init_unit_test_suite(int argc, char* argv[])
{
	boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads: xtime test suite");

	test->add(BOOST_TEST_CASE(&test_xtime_cmp));

	return test;
}