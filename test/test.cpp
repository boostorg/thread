#include <boost/test/unit_test.hpp>

extern boost::unit_test_framework::test_suite* thread_tests();
extern boost::unit_test_framework::test_suite* mutex_tests();
extern boost::unit_test_framework::test_suite* condition_tests();
extern boost::unit_test_framework::test_suite* tss_tests();
extern boost::unit_test_framework::test_suite* once_tests();

boost::unit_test_framework::test_suite* init_unit_test_suite(int argc, char* argv[])
{
	boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads test suite");

	test->add(thread_tests());
    test->add(mutex_tests());
	test->add(condition_tests());
    test->add(tss_tests());
    test->add(once_tests());

    return test;
}
