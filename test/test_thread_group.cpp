#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/test/test_tools.hpp>

#include <utils.inl>

namespace {

int test_value;

void simple_thread()
{
	test_value = 999;
}

void test_create_thread()
{
	test_value = 0;
	boost::thread_group thrds;
	BOOST_TEST(thrds.create_thread(&simple_thread) != 0);
	thrds.join_all();
	BOOST_TEST(test_value == 999);
}

void test_add_find_remove()
{
	test_value = 0;
	boost::thread_group thrds;
	boost::thread* pthread = new boost::thread(&simple_thread);
	thrds.add_thread(pthread);
	BOOST_TEST(thrds.find(*pthread) == pthread);
	thrds.remove_thread(pthread);
	BOOST_TEST(thrds.find(*pthread) == 0);
	pthread->join();
	BOOST_TEST(test_value == 999);
}

} // namespace

void test_thread_group()
{
	test_create_thread();
	test_add_find_remove();
}