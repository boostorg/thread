#include <boost/thread/once.hpp>
#include <boost/thread/thread.hpp>

#include <boost/test/unit_test.hpp>

namespace
{
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
}

void test_once()
{
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_once_thread);
    threads.join_all();
    BOOST_CHECK_EQUAL(once_value, 1);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads: once test suite");

    test->add(BOOST_TEST_CASE(test_once));

    return test;
}
