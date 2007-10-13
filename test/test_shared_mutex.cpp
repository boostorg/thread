// (C) Copyright 2006-7 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/xtime.hpp>
#include "util.inl"

#define CHECK_LOCKED_VALUE_EQUAL(mutex_name,value,expected_value)    \
    {                                                                \
        boost::mutex::scoped_lock lock(mutex_name);                  \
        BOOST_CHECK_EQUAL(value,expected_value);                     \
    }


namespace
{
    template<typename lock_type>
    class locking_thread
    {
        boost::shared_mutex& rw_mutex;
        unsigned& unblocked_count;
        unsigned& simultaneous_running_count;
        unsigned& max_simultaneous_running;
        boost::mutex& unblocked_count_mutex;
        boost::mutex& finish_mutex;
    public:
        locking_thread(boost::shared_mutex& rw_mutex_,
                       unsigned& unblocked_count_,
                       boost::mutex& unblocked_count_mutex_,
                       boost::mutex& finish_mutex_,
                       unsigned& simultaneous_running_count_,
                       unsigned& max_simultaneous_running_):
            rw_mutex(rw_mutex_),
            unblocked_count(unblocked_count_),
            unblocked_count_mutex(unblocked_count_mutex_),
            finish_mutex(finish_mutex_),
            simultaneous_running_count(simultaneous_running_count_),
            max_simultaneous_running(max_simultaneous_running_)
        {}
        
        void operator()()
        {
            // acquire lock
            lock_type lock(rw_mutex);
            
            // increment count to show we're unblocked
            {
                boost::mutex::scoped_lock ublock(unblocked_count_mutex);
                ++unblocked_count;
                ++simultaneous_running_count;
                if(simultaneous_running_count>max_simultaneous_running)
                {
                    max_simultaneous_running=simultaneous_running_count;
                }
            }
            
            // wait to finish
            boost::mutex::scoped_lock finish_lock(finish_mutex);
            {
                boost::mutex::scoped_lock ublock(unblocked_count_mutex);
                --simultaneous_running_count;
            }
        }
    };
    
}


void test_multiple_readers()
{
    unsigned const number_of_threads=100;
    
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    for(unsigned i=0;i<number_of_threads;++i)
    {
        pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    }

    boost::thread::sleep(delay(1));

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);

    finish_lock.unlock();

    pool.join_all();

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,number_of_threads);
}

void test_only_one_writer_permitted()
{
    unsigned const number_of_threads=100;
    
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    for(unsigned i=0;i<number_of_threads;++i)
    {
        pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    }

    boost::thread::sleep(delay(1));

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

    finish_lock.unlock();

    pool.join_all();

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1);
}

void test_reader_blocks_writer()
{
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);
    pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

    finish_lock.unlock();

    pool.join_all();

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,2U);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1);
}

void test_unlocking_writer_unblocks_all_readers()
{
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    boost::unique_lock<boost::shared_mutex>  write_lock(rw_mutex);
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);

    unsigned const reader_count=100;

    for(unsigned i=0;i<reader_count;++i)
    {
        pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    }
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,0U);

    write_lock.unlock();
    
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count);

    finish_lock.unlock();
    pool.join_all();
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,reader_count);
}

void test_unlocking_last_reader_only_unblocks_one_writer()
{
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_readers=0;
    unsigned max_simultaneous_readers=0;
    unsigned simultaneous_running_writers=0;
    unsigned max_simultaneous_writers=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_reading_mutex;
    boost::mutex::scoped_lock finish_reading_lock(finish_reading_mutex);
    boost::mutex finish_writing_mutex;
    boost::mutex::scoped_lock finish_writing_lock(finish_writing_mutex);

    unsigned const reader_count=100;
    unsigned const writer_count=100;

    for(unsigned i=0;i<reader_count;++i)
    {
        pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_reading_mutex,simultaneous_running_readers,max_simultaneous_readers));
    }
    for(unsigned i=0;i<writer_count;++i)
    {
        pool.create_thread(locking_thread<boost::unique_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_writing_mutex,simultaneous_running_writers,max_simultaneous_writers));
    }
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count);

    finish_reading_lock.unlock();

    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+1);

    finish_writing_lock.unlock();
    pool.join_all();
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+writer_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_readers,reader_count);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_writers,1);
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
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    for(unsigned i=0;i<number_of_threads;++i)
    {
        pool.create_thread(locking_thread<boost::upgrade_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    }

    boost::thread::sleep(delay(1));

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,1U);

    finish_lock.unlock();

    pool.join_all();

    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,number_of_threads);
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,max_simultaneous_running,1);
}

void test_can_lock_upgrade_if_currently_locked_shared()
{
    boost::thread_group pool;

    boost::shared_mutex rw_mutex;
    unsigned unblocked_count=0;
    unsigned simultaneous_running_count=0;
    unsigned max_simultaneous_running=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);

    unsigned const reader_count=100;

    for(unsigned i=0;i<reader_count;++i)
    {
        pool.create_thread(locking_thread<boost::shared_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    }
    pool.create_thread(locking_thread<boost::upgrade_lock<boost::shared_mutex> >(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex,simultaneous_running_count,max_simultaneous_running));
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_count_mutex,unblocked_count,reader_count+1);

    finish_lock.unlock();
    pool.join_all();
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
    boost::thread::sleep(delay(1));
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1);

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
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1);

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
    CHECK_LOCKED_VALUE_EQUAL(unblocked_mutex,unblocked_count,1);

    boost::system_time const start=boost::get_system_time();
    boost::system_time const timeout=start+boost::posix_time::milliseconds(100);
    bool const timed_lock_succeeded=rw_mutex.timed_lock_shared(timeout);
    BOOST_CHECK(timeout<=boost::get_system_time());
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

    test->add(BOOST_TEST_CASE(&test_multiple_readers));
    test->add(BOOST_TEST_CASE(&test_only_one_writer_permitted));
    test->add(BOOST_TEST_CASE(&test_reader_blocks_writer));
    test->add(BOOST_TEST_CASE(&test_unlocking_writer_unblocks_all_readers));
    test->add(BOOST_TEST_CASE(&test_unlocking_last_reader_only_unblocks_one_writer));
    test->add(BOOST_TEST_CASE(&test_only_one_upgrade_lock_permitted));
    test->add(BOOST_TEST_CASE(&test_can_lock_upgrade_if_currently_locked_shared));
    test->add(BOOST_TEST_CASE(&test_if_other_thread_has_write_lock_try_lock_shared_returns_false));
    test->add(BOOST_TEST_CASE(&test_if_no_thread_has_lock_try_lock_shared_returns_true));
    test->add(BOOST_TEST_CASE(&test_if_other_thread_has_shared_lock_try_lock_shared_returns_true));
    test->add(BOOST_TEST_CASE(&test_timed_lock_shared_times_out_if_write_lock_held));

    return test;
}
