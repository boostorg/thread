#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/test/test_tools.hpp>

namespace {

template <typename M>
void test_lock(M* dummy=0)
{
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    lock_type lock(mutex);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock and unlock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
}

template <typename M>
void test_trylock(M* dummy=0)
{
	typedef M mutex_type;
    typedef typename M::scoped_try_lock try_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        try_lock_type lock(mutex);
        BOOST_TEST(lock);
    }
    {
        try_lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    try_lock_type lock(mutex, true);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock, unlock and trylock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
    lock.unlock();
    BOOST_TEST(!lock);
    BOOST_TEST(lock.try_lock());
    BOOST_TEST(lock);
}

template <typename M>
void test_timedlock(M* dummy=0)
{
    typedef M mutex_type;
    typedef typename M::scoped_timed_lock timed_lock_type;

    mutex_type mutex;
    boost::condition condition;

    // Test the lock's constructors.
    {
        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt;
        BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
        xt.nsec += 100000000;

        timed_lock_type lock(mutex, xt);
        BOOST_TEST(lock);
    }
    {
        timed_lock_type lock(mutex, false);
        BOOST_TEST(!lock);
    }
    timed_lock_type lock(mutex, true);
    BOOST_TEST(lock);

    // Construct and initialize an xtime for a fast time out.
    boost::xtime xt;
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;

    // Test the lock and the mutex with condition variables.
    // No one is going to notify this condition variable.  We expect to
    // time out.
    BOOST_TEST(condition.timed_wait(lock, xt) == false);
    BOOST_TEST(lock);

    // Test the lock, unlock and timedlock methods.
    lock.unlock();
    BOOST_TEST(!lock);
    lock.lock();
    BOOST_TEST(lock);
    lock.unlock();
    BOOST_TEST(!lock);
    BOOST_TEST(boost::xtime_get(&xt, boost::TIME_UTC) == boost::TIME_UTC);
    xt.nsec += 100000000;
    BOOST_TEST(lock.timed_lock(xt));
}

} // namespace

void test_mutex()
{
    typedef boost::mutex mutex;
    test_lock<mutex>();
}

void test_try_mutex()
{
    typedef boost::try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
}

void test_timed_mutex()
{
    typedef boost::timed_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    test_timedlock<mutex>();
}

void test_recursive_mutex()
{
    typedef boost::recursive_mutex mutex;
    test_lock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_try_mutex()
{
    typedef boost::recursive_try_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}

void test_recursive_timed_mutex()
{
    typedef boost::recursive_timed_mutex mutex;
    test_lock<mutex>();
    test_trylock<mutex>();
    test_timedlock<mutex>();
    mutex mx;
    mutex::scoped_lock lock1(mx);
    mutex::scoped_lock lock2(mx);
}
