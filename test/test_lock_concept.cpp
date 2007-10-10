// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/vector.hpp>
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
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_initially_unlocked_with_defer_lock_parameter
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);
        
        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_initially_locked_with_adopt_lock_parameter
{
    void operator()() const
    {
        Mutex m;
        m.lock();
        Lock lock(m,boost::adopt_lock);
        
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};


template<typename Mutex,typename Lock>
struct test_unlocked_after_unlock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        lock.unlock();
        BOOST_CHECK(!lock);
        BOOST_CHECK(!lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_locked_after_lock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);
        lock.lock();
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_locked_after_try_lock_called
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m,boost::defer_lock);
        lock.try_lock();
        BOOST_CHECK(lock);
        BOOST_CHECK(lock.owns_lock());
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_lock_called_when_already_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        
        BOOST_CHECK_THROW( lock.lock(), boost::lock_error );
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_try_lock_called_when_already_locked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        
        BOOST_CHECK_THROW( lock.try_lock(), boost::lock_error );
    }
};

template<typename Mutex,typename Lock>
struct test_throws_if_unlock_called_when_already_unlocked
{
    void operator()() const
    {
        Mutex m;
        Lock lock(m);
        lock.unlock();
        
        BOOST_CHECK_THROW( lock.unlock(), boost::lock_error );
    }
};

BOOST_TEST_CASE_TEMPLATE_FUNCTION(test_scoped_lock_concept,Mutex)
{
    typedef typename Mutex::scoped_lock Lock;
    
    test_initially_locked<Mutex,Lock>()();
    test_initially_unlocked_with_defer_lock_parameter<Mutex,Lock>()();
    test_initially_locked_with_adopt_lock_parameter<Mutex,Lock>()();
    test_unlocked_after_unlock_called<Mutex,Lock>()();
    test_locked_after_lock_called<Mutex,Lock>()();
    test_throws_if_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>()();
}

BOOST_TEST_CASE_TEMPLATE_FUNCTION(test_scoped_try_lock_concept,Mutex)
{
    typedef typename Mutex::scoped_try_lock Lock;
    
    test_initially_locked<Mutex,Lock>()();
    test_initially_unlocked_with_defer_lock_parameter<Mutex,Lock>()();
    test_initially_locked_with_adopt_lock_parameter<Mutex,Lock>()();
    test_unlocked_after_unlock_called<Mutex,Lock>()();
    test_locked_after_lock_called<Mutex,Lock>()();
    test_locked_after_try_lock_called<Mutex,Lock>()();
    test_throws_if_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_try_lock_called_when_already_locked<Mutex,Lock>()();
    test_throws_if_unlock_called_when_already_unlocked<Mutex,Lock>()();
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: lock concept test suite");

    typedef boost::mpl::vector<boost::mutex,boost::try_mutex,boost::timed_mutex,
        boost::recursive_mutex,boost::recursive_try_mutex,boost::recursive_timed_mutex> mutex_types;
    
    test->add(BOOST_TEST_CASE_TEMPLATE(test_scoped_lock_concept,mutex_types));

    typedef boost::mpl::vector<boost::try_mutex,boost::timed_mutex,
        boost::recursive_try_mutex,boost::recursive_timed_mutex> try_mutex_types;

    test->add(BOOST_TEST_CASE_TEMPLATE(test_scoped_try_lock_concept,try_mutex_types));

    return test;
}
