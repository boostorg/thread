#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/test/test_tools.hpp>

namespace {

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
//boost::thread_specific_ptr<tss_value_t> tss_value_no_delete(0);

void test_tss_thread()
{
	BOOST_TEST(tss_value.get() == 0);
	tss_value_t* p = new tss_value_t();
    tss_value.reset(p);
	BOOST_TEST(tss_value.get() == p);

    for (int i=0; i<1000; ++i)
    {
        BOOST_TEST(tss_value->value == i);
        ++tss_value->value;
    }
}

} // namespace

void test_thread_specific_ptr()
{
    /*const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_tss_thread);
    threads.join_all();
    BOOST_TEST(tss_instances == 0);*/
}
