#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
//#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <utils.inl>

namespace
{
    int test_value;

    void simple_thread()
    {
        test_value = 999;
    }

    void cancel_thread()
    {
		// This block will test the cancellation guard. If it
		// doesn't work, we'll be cancelled with out setting
		// the test_value to 999.
		try
		{
			boost::cancellation_guard guard;
			// Sleep long enough to let the main thread cancel us
			boost::thread::sleep(xtime_get_future(3));
		}
		catch (boost::thread_cancel& cancel)
		{
			test_value = 666; // indicates unexpected cancellation
			throw; // Make sure to re-throw!
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

	struct indirect_adapter
	{
		indirect_adapter(void (*func)()) : func(func) { }
		void operator()()
		{
			boost::thread thrd(func);
			thrd.join();
		}
		void (*func)();
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
    boost::xtime xt = xtime_get_future(3);
    boost::thread::sleep(xt);

    // Insure it's in a range instead of checking actual equality due to time
    // lapse
    BOOST_CHECK(xtime_in_range(xt, -1, 0));
}

void test_creation()
{
    test_value = 0;
    boost::thread thrd(&simple_thread);
	boost::thread::sleep(xtime_get_future(2));
    BOOST_CHECK_EQUAL(test_value, 999);
}

void test_join()
{
	test_value = 0;
	boost::thread thrd = boost::thread(indirect_adapter(&simple_thread));
	boost::thread::sleep(xtime_get_future(2));
	BOOST_CHECK_EQUAL(test_value, 999);
}

void test_comparison()
{
    boost::thread self;
	BOOST_CHECK(self == boost::thread());

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
	thrd.join();
	BOOST_CHECK_EQUAL(test_value, 999); // only true if thread was cancelled
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread test suite");

    test->add(BOOST_TEST_CASE(test_sleep));
    test->add(BOOST_TEST_CASE(test_creation));
    test->add(BOOST_TEST_CASE(test_comparison));
	test->add(BOOST_TEST_CASE(test_join));
	test->add(BOOST_TEST_CASE(test_cancel));

    return test;
}
