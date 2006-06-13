// (C) Copyright 2006 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>

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

template<typename Mutex,typename Lock>
struct test_throws_if_unlock_called_when_already_unlocked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,true);
        lock.unlock();
        
        BOOST_CHECK_THROW( lock.unlock(), boost::lock_error );
    }
};

template<typename Mutex>
void add_tests_for_scoped_lock_concept(boost::unit_test_framework::test_suite* test,Mutex* =0)
{
    typedef typename Mutex::scoped_lock Lock;
    
    test->add(BOOST_TEST_CASE((test_initially_locked<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_initially_locked_with_bool_parameter_true<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_initially_unlocked_with_bool_parameter_false<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_unlocked_after_unlock_called<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_locked_after_lock_called<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_throws_if_lock_called_when_already_locked<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>())));
}

template<typename Mutex>
void add_tests_for_scoped_try_lock_concept(boost::unit_test_framework::test_suite* test,Mutex* =0)
{
    typedef typename Mutex::scoped_try_lock Lock;
    
    test->add(BOOST_TEST_CASE((test_initially_locked<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_initially_locked_with_bool_parameter_true<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_initially_unlocked_with_bool_parameter_false<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_unlocked_after_unlock_called<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_locked_after_lock_called<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_throws_if_lock_called_when_already_locked<Mutex,Lock>())));
    test->add(BOOST_TEST_CASE((test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>())));
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: lock concept test suite");

    add_tests_for_scoped_lock_concept<boost::mutex>(test);
    add_tests_for_scoped_lock_concept<boost::try_mutex>(test);
    add_tests_for_scoped_lock_concept<boost::recursive_mutex>(test);

    add_tests_for_scoped_try_lock_concept<boost::try_mutex>(test);

    return test;
}
