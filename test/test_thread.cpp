#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
//#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

namespace
{
    inline bool xtime_in_range(boost::xtime& xt, int less_seconds,
        int greater_seconds)
    {
        boost::xtime cur;
        BOOST_CHECK_EQUAL(boost::xtime_get(&cur, boost::TIME_UTC),
            static_cast<int>(boost::TIME_UTC));

        boost::xtime less = cur;
        less.sec += less_seconds;

        boost::xtime greater = cur;
        greater.sec += greater_seconds;

        return (boost::xtime_cmp(xt, less) >= 0) &&
            (boost::xtime_cmp(xt, greater) <= 0);
    }

    int test_value;

    void simple_thread()
    {
        test_value = 999;
    }

    void cancel_thread()
    {
		// Sleep long enough to let the main thread cancel us
		boost::xtime xt;
		BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC),
			static_cast<int>(boost::TIME_UTC));
		xt.sec += 3;
		boost::thread::sleep(xt);

		// This block will test the cancellation guard. If it
		// doesn't work, we'll be cancelled with out setting
		// the test_value to 999.
		{
			boost::cancellation_guard guard;
			boost::thread::test_cancel();
		}

		// This block tests the cancellation itself.  If it
		// works a thread_cancel exception will be thrown,
		// and in the catch handler for it we'll set our
		// exptected test_value of 999.
		try
		{
			boost::thread::test_cancel();
		}
		catch (boost::thread_cancel& cancel)
		{
			test_value = 999;
			throw; // Make sure to re-throw!
		}
    }

    struct thread_adapter
    {
        thread_adapter(void (*func)(boost::thread& parent),
            boost::thread& parent) : func(func), parent(parent)
        {
        }

        void operator()()
        {
            (*func)(parent);
        }

        void (*func)(boost::thread& parent);
        boost::thread& parent;
    };

    void comparison_thread(boost::thread& parent)
    {
        boost::thread thrd;
        BOOST_TEST(thrd != parent);
        BOOST_TEST(thrd == boost::thread());
    }
}

void test_sleep()
{
    boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC),
        static_cast<int>(boost::TIME_UTC));
    xt.sec += 3;

    boost::thread::sleep(xt);

    // Insure it's in a range instead of checking actual equality due to time
    // lapse
    BOOST_CHECK(xtime_in_range(xt, -1, 0));
}

void test_creation()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
    thrd.join();
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_comparison()
{
    boost::thread self;
	BOOST_CHECK_EQUAL(self, boost::thread());

    boost::thread thrd(thread_adapter(comparison_thread, self));
	boost::thread thrd2 = thrd;

	BOOST_CHECK(thrd != self);
	BOOST_CHECK(thrd == thrd2);

    thrd.join();
}

void test_cancel()
{
	test_value = 0;
	boost::thread thrd(&cancel_thread);
	thrd.cancel();
	BOOST_CHECK_EQUAL(test_value, 999); // only true if thread was cancelled
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_comparison));

    return test;
}
