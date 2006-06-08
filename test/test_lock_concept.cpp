// (C) Copyright 2006 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>

template<typename Mutex,typename Lock>
struct test_initially_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.locked());
    }
};

template<typename Mutex,typename Lock>
struct test_initially_locked_with_bool_parameter_true
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,true);
    
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.locked());
    }
};


template<typename Mutex,typename Lock>
struct test_initially_unlocked_with_bool_parameter_false
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,false);
        
        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.locked());
    }
};


template<typename Mutex,typename Lock>
struct test_unlocked_after_unlock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,true);
        lock.unlock();
        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.locked());
    }
};

template<typename Mutex,typename Lock>
struct test_locked_after_lock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,false);
        lock.lock();
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.locked());
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_lock_called_when_already_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,true);
        
        BOOST_CHECK_THROW( lock.lock(), boost::lock_error );
    }
};

void test_mutex_scoped_lock_throws_if_unlock_called_when_already_unlocked()
{
    boost::mutex m;
    boost::mutex::scoped_lock lock(m,true);
    lock.unlock();
    
    BOOST_CHECK_THROW( lock.unlock(), boost::lock_error );
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: lock concept test suite");

    test->add(BOOST_TEST_CASE((test_initially_locked<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_initially_locked_with_bool_parameter_true<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_initially_unlocked_with_bool_parameter_false<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_unlocked_after_unlock_called<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_locked_after_lock_called<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_throws_if_lock_called_when_already_locked<boost::mutex,boost::mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE(&test_mutex_scoped_lock_throws_if_unlock_called_when_already_unlocked));

    test->add(BOOST_TEST_CASE((test_initially_locked<boost::try_mutex,boost::try_mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_initially_locked_with_bool_parameter_true<boost::try_mutex,boost::try_mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_initially_unlocked_with_bool_parameter_false<boost::try_mutex,boost::try_mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_unlocked_after_unlock_called<boost::try_mutex,boost::try_mutex::scoped_lock>())));
    test->add(BOOST_TEST_CASE((test_locked_after_lock_called<boost::try_mutex,boost::try_mutex::scoped_lock>())));

    test->add(BOOST_TEST_CASE((test_initially_locked_with_bool_parameter_true<boost::try_mutex,boost::try_mutex::scoped_try_lock>())));
    test->add(BOOST_TEST_CASE((test_initially_unlocked_with_bool_parameter_false<boost::try_mutex,boost::try_mutex::scoped_try_lock>())));
    test->add(BOOST_TEST_CASE((test_unlocked_after_unlock_called<boost::try_mutex,boost::try_mutex::scoped_try_lock>())));
    test->add(BOOST_TEST_CASE((test_locked_after_lock_called<boost::try_mutex,boost::try_mutex::scoped_try_lock>())));

    return test;
}
