#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <boost/test/unit_test.hpp>

#include "util.inl"

boost::mutex tss_mutex;
int tss_instances = 0;

struct tss_value_t
{
    tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        ++tss_instances;
        value = 0;
    }
    ~tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        --tss_instances;
    }
    int value;
};

boost::thread_specific_ptr<tss_value_t> tss_value;

void test_tss_thread()
{
    tss_value.reset(new tss_value_t());
    for (int i=0; i<1000; ++i)
    {
        int& n = tss_value->value;
        BOOST_CHECK_EQUAL(n, i);
        ++n;
    }
}

void do_test_tss()
{
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_tss_thread);
    threads.join_all();
    BOOST_CHECK_EQUAL(tss_instances, 0);
}

void test_tss()
{
	timed_test(&do_test_tss, 2);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads: tss test suite");

    test->add(BOOST_TEST_CASE(test_tss));

    return test;
}
