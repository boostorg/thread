#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/test/test_tools.hpp>

void test_xtime_get()
{
	boost::xtime xt;

	// If this fails we can't really run any tests, so we'll flag a critical error
	BOOST_CRITICAL_TEST(boost::TIME_UTC == boost::xtime_get(&xt, boost::TIME_UTC));
}