#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/tss.hpp>
#include <boost/test/test_tools.hpp>

#include <iostream>

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

boost::thread_specific_ptr<tss_value_t>* tss_value = 0;
//boost::thread_specific_ptr<tss_value_t> tss_value_no_delete(0);

void test_tss_thread()
{
	boost::thread_specific_ptr<tss_value_t>& tss = *tss_value;
	BOOST_TEST(tss.get() == 0);
	tss_value_t* p = new tss_value_t();
    tss.reset(p);
	BOOST_TEST(tss.get() == p);

    for (int i=0; i<1; ++i)
    {
        BOOST_TEST(tss->value == i);
        ++tss->value;
    }
}

} // namespace

void test_thread_specific_ptr()
{
	{
		boost::thread_specific_ptr<tss_value_t> tss;
		tss_value = &tss;

		BOOST_TEST(tss.get() == 0);
		tss_value_t* p = new tss_value_t();
		tss.reset(p);
		BOOST_TEST(tss.get() == p);

		const int NUMTHREADS=2;
		boost::thread_group threads;
		for (int i=0; i<NUMTHREADS; ++i)
			threads.create_thread(&test_tss_thread);
		threads.join_all();
		BOOST_TEST(tss_instances == 1);
	}
	BOOST_TEST(tss_instances == 0);
}
