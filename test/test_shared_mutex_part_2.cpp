// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include "util.inl"
#include "shared_mutex_locking_thread.hpp"

#define CHECK_LOCKED_VALUE_EQUAL(mutex_name,value,expected_value)    \
    {                                                                \
        boost::mutex::scoped_lock lock(mutex_name);                  \
        BOOST_CHECK_EQUAL(value,expected_value);                     \
    }


void test_only_one_upgrade_lock_permitted()
{
    unsigned const number_of_threads=100;
    
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    try
    {
        for(unsigned i=0;i<number_of_threads;++i)
        {
            pool.create_thread(locking_thread<boost::upgrade_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                         finish_mutex,simultaneous_running_count,max_simultaneous_running));
        }

        boost::thread::sleep(delay(1));

        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

        finish_lock.unlock();

        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1u);
}

void test_can_lock_upgrade_if_currently_locked_shared()
{
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::condition_variable unblocked_condition;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);

    unsigned const reader_count=100;

    try
    {
        for(unsigned i=0;i<reader_count;++i)
        {
            pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                        finish_mutex,simultaneous_running_count,max_simultaneous_running));
        }
        boost::thread::sleep(delay(1));
        pool.create_thread(locking_thread<boost::upgrade_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,unblocked_condition,
                                                                                     finish_mutex,simultaneous_running_count,max_simultaneous_running));
        {
            boost::mutex::scoped_lock lk(unblocked_count_mutex);
            while(unblocked_count<(reader_count+1))
            {
                unblocked_condition.wait(lk);
            }
        }
        CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+1);

        finish_lock.unlock();
        pool.join_all();
    }
    catch(...)
    {
        pool.interrupt_all();
        pool.join_all();
        throw;
    }


    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+1);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,reader_count+1);
}

namespace
{
    class simple_writing_thread
    {
        boost::shared_mutex& rwm;
        boost::mutex& finish_mutex;
        boost::mutex& unblocked_mutex;
        unsigned& unblocked_count;
        
        void operator=(simple_writing_thread&);
        
    public:
        simple_writing_thread(boost::shared_mutex& rwm_,
                              boost::mutex& finish_mutex_,
                              boost::mutex& unblocked_mutex_,
                              unsigned& unblocked_count_):
            rwm(rwm_),finish_mutex(finish_mutex_),
            unblocked_mutex(unblocked_mutex_),unblocked_count(unblocked_count_)
        {}
        
        void operator()()
        {
            boost::unique_lock<boost::shared_mutex>  lk(rwm);
            
            {
                boost::mutex::scoped_lock ulk(unblocked_mutex);
                ++unblocked_count;
            }
            
            boost::mutex::scoped_lock flk(finish_mutex);
        }
    };
}

void test_if_other_thread_has_write_lock_try_lock_shared_returns_false()
{

    boost::shared_mutex rw_mutex;
    boost::mutex finish_mutex;
    boost::mutex unblocked_mutex;
    unsigned unblocked_count=0;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    boost::thread writer(simple_writing_thread(rw_mutex,finish_mutex,unblocked_mutex,unblocked_count));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1u);

    bool const try_succeeded=rw_mutex.try_lock_shared();
    BOOST_CHECK(!try_succeeded);
    if(try_succeeded)
    {
        rw_mutex.unlock_shared();
    }

    finish_lock.unlock();
    writer.join();
}

void test_if_no_thread_has_lock_try_lock_shared_returns_true()
{
    boost::shared_mutex rw_mutex;
    bool const try_succeeded=rw_mutex.try_lock_shared();
    BOOST_CHECK(try_succeeded);
    if(try_succeeded)
    {
        rw_mutex.unlock_shared();
    }
}

namespace
{
    class simple_reading_thread
    {
        boost::shared_mutex& rwm;
        boost::mutex& finish_mutex;
        boost::mutex& unblocked_mutex;
        unsigned& unblocked_count;
        
        void operator=(simple_reading_thread&);
        
    public:
        simple_reading_thread(boost::shared_mutex& rwm_,
                              boost::mutex& finish_mutex_,
                              boost::mutex& unblocked_mutex_,
                              unsigned& unblocked_count_):
            rwm(rwm_),finish_mutex(finish_mutex_),
            unblocked_mutex(unblocked_mutex_),unblocked_count(unblocked_count_)
        {}
        
        void operator()()
        {
            boost::shared_lock<boost::shared_mutex>  lk(rwm);
            
            {
                boost::mutex::scoped_lock ulk(unblocked_mutex);
                ++unblocked_count;
            }
            
            boost::mutex::scoped_lock flk(finish_mutex);
        }
    };
}

void test_if_other_thread_has_shared_lock_try_lock_shared_returns_true()
{

    boost::shared_mutex rw_mutex;
    boost::mutex finish_mutex;
    boost::mutex unblocked_mutex;
    unsigned unblocked_count=0;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    boost::thread writer(simple_reading_thread(rw_mutex,finish_mutex,unblocked_mutex,unblocked_count));
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1u);

    bool const try_succeeded=rw_mutex.try_lock_shared();
    BOOST_CHECK(try_succeeded);
    if(try_succeeded)
    {
        rw_mutex.unlock_shared();
    }

    finish_lock.unlock();
    writer.join();
}

void test_timed_lock_shared_times_out_if_write_lock_held()
{
    boost::shared_mutex rw_mutex;
    boost::mutex finish_mutex;
    boost::mutex unblocked_mutex;
    unsigned unblocked_count=0;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    boost::thread writer(simple_writing_thread(rw_mutex,finish_mutex,unblocked_mutex,unblocked_count));
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1u);

    boost::system_time const start=boost::get_system_time();
    boost::system_time const timeout=start+boost::posix_time::milliseconds(2000);
    boost::posix_time::milliseconds const timeout_resolution(20);
    bool const timed_lock_succeeded=rw_mutex.timed_lock_shared(timeout);
    BOOST_CHECK((timeout-timeout_resolution)<boost::get_system_time());
    BOOST_CHECK(!timed_lock_succeeded);
    if(timed_lock_succeeded)
    {
        rw_mutex.unlock_shared();
    }

    finish_lock.unlock();
    writer.join();
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: shared_mutex test suite");

    test->add(BOOST_TEST_CASE(&test_only_one_upgrade_lock_permitted));
    test->add(BOOST_TEST_CASE(&test_can_lock_upgrade_if_currently_locked_shared));
    test->add(BOOST_TEST_CASE(&test_if_other_thread_has_write_lock_try_lock_shared_returns_false));
    test->add(BOOST_TEST_CASE(&test_if_no_thread_has_lock_try_lock_shared_returns_true));
    test->add(BOOST_TEST_CASE(&test_if_other_thread_has_shared_lock_try_lock_shared_returns_true));
    test->add(BOOST_TEST_CASE(&test_timed_lock_shared_times_out_if_write_lock_held));

    return test;
}
