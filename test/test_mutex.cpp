#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite_ex.hpp>

template <typename M>
struct test_lock
{
	typedef M mutex_type;
	typedef typename M::scoped_lock lock_type;

	void operator()()
	{
		mutex_type mutex;
		boost::condition condition;

		// Test the lock's constructors.
		{
			lock_type lock(mutex, false);
			BOOST_CHECK(!lock);
		}
		lock_type lock(mutex);
		BOOST_CHECK(lock);

		// Construct and initialize an xtime for a fast time out.
		boost::xtime xt;
		BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
		xt.nsec += 100000000;

		// Test the lock and the mutex with condition variables.
		// No one is going to notify this condition variable.  We expect to
		// time out.
		BOOST_CHECK(!condition.timed_wait(lock, xt));
		BOOST_CHECK(lock);

		// Test the lock and unlock methods.
		lock.unlock();
		BOOST_CHECK(!lock);
		lock.lock();
		BOOST_CHECK(lock);
	}
};

template <typename M>
struct test_trylock
{
    typedef M mutex_type;
    typedef typename M::scoped_try_lock try_lock_type;

	void operator()()
	{
		mutex_type mutex;
		boost::condition condition;

		// Test the lock's constructors.
		{
			try_lock_type lock(mutex);
			BOOST_CHECK(lock);
		}
		{
			try_lock_type lock(mutex, false);
			BOOST_CHECK(!lock);
		}
		try_lock_type lock(mutex, true);
		BOOST_CHECK(lock);

		// Construct and initialize an xtime for a fast time out.
		boost::xtime xt;
		BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
		xt.nsec += 100000000;

		// Test the lock and the mutex with condition variables.
		// No one is going to notify this condition variable.  We expect to
		// time out.
		BOOST_CHECK(!condition.timed_wait(lock, xt));
		BOOST_CHECK(lock);

		// Test the lock, unlock and trylock methods.
		lock.unlock();
		BOOST_CHECK(!lock);
		lock.lock();
		BOOST_CHECK(lock);
		lock.unlock();
		BOOST_CHECK(!lock);
		BOOST_CHECK(lock.try_lock());
		BOOST_CHECK(lock);
	}
};

template <typename M>
struct test_timedlock
{
    typedef M mutex_type;
    typedef typename M::scoped_timed_lock timed_lock_type;

	void operator()()
	{
		mutex_type mutex;
		boost::condition condition;

		// Test the lock's constructors.
		{
			// Construct and initialize an xtime for a fast time out.
			boost::xtime xt;
			BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
			xt.nsec += 100000000;

			timed_lock_type lock(mutex, xt);
			BOOST_CHECK(lock);
		}
		{
			timed_lock_type lock(mutex, false);
			BOOST_CHECK(!lock);
		}
		timed_lock_type lock(mutex, true);
		BOOST_CHECK(lock);

		// Construct and initialize an xtime for a fast time out.
		boost::xtime xt;
		BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
		xt.nsec += 100000000;

		// Test the lock and the mutex with condition variables.
		// No one is going to notify this condition variable.  We expect to
		// time out.
		BOOST_CHECK(!condition.timed_wait(lock, xt));
		BOOST_CHECK(lock);

		// Test the lock, unlock and timedlock methods.
		lock.unlock();
		BOOST_CHECK(!lock);
		lock.lock();
		BOOST_CHECK(lock);
		lock.unlock();
		BOOST_CHECK(!lock);
		BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC), boost::TIME_UTC);
		xt.nsec += 100000000;
		BOOST_CHECK(lock.timed_lock(xt));
	}
};

template <typename M>
struct test_recursive_lock
{
	typedef M mutex;

	void operator()()
	{
		mutex mx;
		mutex::scoped_lock lock1(mx);
		mutex::scoped_lock lock2(mx);
	}
};

boost::unit_test_framework::test_suite* init_unit_test_suite(int argc, char* argv[])
{
	boost::unit_test_framework::test_suite* test = BOOST_TEST_SUITE("Boost.Threads: mutex test suite");

	test->add(BOOST_TEST_CASE(test_lock<boost::mutex>()));

	test->add(BOOST_TEST_CASE(test_lock<boost::try_mutex>()));
	test->add(BOOST_TEST_CASE(test_trylock<boost::try_mutex>()));

	test->add(BOOST_TEST_CASE(test_lock<boost::timed_mutex>()));
	test->add(BOOST_TEST_CASE(test_trylock<boost::timed_mutex>()));
	test->add(BOOST_TEST_CASE(test_timedlock<boost::timed_mutex>()));

	test->add(BOOST_TEST_CASE(test_lock<boost::recursive_mutex>()));
	test->add(BOOST_TEST_CASE(test_recursive_lock<boost::recursive_mutex>()));

	test->add(BOOST_TEST_CASE(test_lock<boost::recursive_try_mutex>()));
	test->add(BOOST_TEST_CASE(test_trylock<boost::recursive_try_mutex>()));
	test->add(BOOST_TEST_CASE(test_recursive_lock<boost::recursive_try_mutex>()));

	test->add(BOOST_TEST_CASE(test_lock<boost::recursive_timed_mutex>()));
	test->add(BOOST_TEST_CASE(test_trylock<boost::recursive_timed_mutex>()));
	test->add(BOOST_TEST_CASE(test_timedlock<boost::recursive_timed_mutex>()));
	test->add(BOOST_TEST_CASE(test_recursive_lock<boost::recursive_timed_mutex>()));

	return test;
}