#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/test/test_tools.hpp>

#include <utils.inl>

namespace {

int test_value;

struct thread_comparison_adapter
{
	thread_comparison_adapter(void (*func)(boost::thread& parent), boost::thread& parent)
		: func(func), parent(parent)
	{
	}

	void operator()()
	{
		(*func)(parent);
	}

	void (*func)(boost::thread& parent);
	boost::thread& parent;
};

void test_sleep()
{
	boost::xtime xt;
	xtime_get(xt, 5);
	boost::thread::sleep(xt);
	// Insure it's in a range instead of checking actual equality due to time lapse
	BOOST_TEST(xtime_in_range(xt, -1, 1));
}

void simple_thread()
{
	test_value = 999;
}

void test_creation()
{
	try
	{
		test_value = 0;
		boost::thread thrd(&simple_thread);
		thrd.join();
		// If creation fails there's little point in continuing...
		BOOST_CRITICAL_TEST(test_value == 999);
	}
	catch (boost::thread_resource_error& err)
	{
		BOOST_CRITICAL_ERROR("Caught thread_resource_error");
	}
}

void comparison_thread(boost::thread& parent)
{
	boost::thread thrd;
	BOOST_TEST(thrd != parent);
	BOOST_TEST(thrd == boost::thread());
}

void test_comparison()
{
	boost::thread self;
	boost::thread thrd(thread_comparison_adapter(&comparison_thread, self));
	thrd.join();
}

} // namespace

void test_thread()
{
	test_sleep();
	test_creation();
	test_comparison();
}