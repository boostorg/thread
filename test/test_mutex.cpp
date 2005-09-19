// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//
// (C) Copyright 2005 Anthony Williams


#include <boost/thread/detail/config.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread.hpp>

#include <boost/test/unit_test.hpp>

#define DEFAULT_EXECUTION_MONITOR_TYPE execution_monitor::use_sleep_only
#include <libs/thread/test/util.inl>

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
        BOOST_CHECK(lock ? true : false);

        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt = delay(0, 100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, xt));
        BOOST_CHECK(lock ? true : false);

        // Test the lock and unlock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
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
            BOOST_CHECK(lock ? true : false);
        }
        {
            try_lock_type lock(mutex, false);
            BOOST_CHECK(!lock);
        }
        try_lock_type lock(mutex, true);
        BOOST_CHECK(lock ? true : false);

        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt = delay(0, 100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, xt));
        BOOST_CHECK(lock ? true : false);

        // Test the lock, unlock and trylock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
        lock.unlock();
        BOOST_CHECK(!lock);
        BOOST_CHECK(lock.try_lock());
        BOOST_CHECK(lock ? true : false);
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
            boost::xtime xt = delay(0, 100);

            timed_lock_type lock(mutex, xt);
            BOOST_CHECK(lock ? true : false);
        }
        {
            timed_lock_type lock(mutex, false);
            BOOST_CHECK(!lock);
        }
        timed_lock_type lock(mutex, true);
        BOOST_CHECK(lock ? true : false);

        // Construct and initialize an xtime for a fast time out.
        boost::xtime xt = delay(0, 100);

        // Test the lock and the mutex with condition variables.
        // No one is going to notify this condition variable.  We expect to
        // time out.
        BOOST_CHECK(!condition.timed_wait(lock, xt));
        BOOST_CHECK(lock ? true : false);
        BOOST_CHECK(in_range(xt));

        // Test the lock, unlock and timedlock methods.
        lock.unlock();
        BOOST_CHECK(!lock);
        lock.lock();
        BOOST_CHECK(lock ? true : false);
        lock.unlock();
        BOOST_CHECK(!lock);
        xt = delay(0, 100);
        BOOST_CHECK(lock.timed_lock(xt));
        BOOST_CHECK(lock ? true : false);
    }
};

template <typename M>
struct test_recursive_lock
{
    typedef M mutex_type;
    typedef typename M::scoped_lock lock_type;

    void operator()()
    {
        mutex_type mx;
        lock_type lock1(mx);
        lock_type lock2(mx);
    }
};

void do_test_mutex()
{
    test_lock<boost::mutex>()();
}

void test_mutex()
{
    timed_test(&do_test_mutex, 3);
}

void do_test_try_mutex()
{
    test_lock<boost::try_mutex>()();
    test_trylock<boost::try_mutex>()();
}

void test_try_mutex()
{
    timed_test(&do_test_try_mutex, 3);
}

void do_test_timed_mutex()
{
    test_lock<boost::timed_mutex>()();
    test_trylock<boost::timed_mutex>()();
    test_timedlock<boost::timed_mutex>()();
}

void test_timed_mutex()
{
    timed_test(&do_test_timed_mutex, 3);
}

void do_test_recursive_mutex()
{
    test_lock<boost::recursive_mutex>()();
    test_recursive_lock<boost::recursive_mutex>()();
}

void test_recursive_mutex()
{
    timed_test(&do_test_recursive_mutex, 3);
}

void do_test_recursive_try_mutex()
{
    test_lock<boost::recursive_try_mutex>()();
    test_trylock<boost::recursive_try_mutex>()();
    test_recursive_lock<boost::recursive_try_mutex>()();
}

void test_recursive_try_mutex()
{
    timed_test(&do_test_recursive_try_mutex, 3);
}

void do_test_recursive_timed_mutex()
{
    test_lock<boost::recursive_timed_mutex>()();
    test_trylock<boost::recursive_timed_mutex>()();
    test_timedlock<boost::recursive_timed_mutex>()();
    test_recursive_lock<boost::recursive_timed_mutex>()();
}

void test_recursive_timed_mutex()
{
    timed_test(&do_test_recursive_timed_mutex, 3);
}

namespace
{
    template<typename Mutex>
    class loop_on_mutex
    {
        Mutex& m;
        unsigned loop_count;
        unsigned& counter;
    public:
        loop_on_mutex(Mutex& m_,unsigned loop_count_,unsigned& counter_):
            m(m_),loop_count(loop_count_),counter(counter_)
        {}
        void operator()()
        {
            for(unsigned i=0;i<loop_count;++i)
            {
                typename Mutex::scoped_lock lock(m);
                ++counter;
            }
        }
    };
    
    template<typename Mutex>
    void test_loop_threads()
    {
        Mutex m;

        unsigned const number_of_threads=100;
        unsigned const loop_count=100;
        unsigned count=0;
    
        boost::thread_group pool;

        for (unsigned i=0; i < number_of_threads; ++i)
        {
            pool.create_thread( loop_on_mutex<Mutex>(m,loop_count,count) );
        }
    
        pool.join_all();
    
        BOOST_CHECK(count==number_of_threads*loop_count);
    }
}

void test_loop_threads_on_mutex()
{
    test_loop_threads<boost::mutex>();
    test_loop_threads<boost::try_mutex>();
    test_loop_threads<boost::timed_mutex>();
}

namespace
{
    template<typename Mutex>
    class block_on_mutex
    {
        Mutex& mutex;
        unsigned& unblocked_count;
        boost::mutex& unblocked_count_mutex;
        boost::mutex& finish_mutex;

        void increment_unblocked_count()
        {
            boost::mutex::scoped_lock lock(unblocked_count_mutex);
            ++unblocked_count;
        }
    public:
        block_on_mutex(Mutex& mutex_,
                       unsigned& unblocked_count_,
                       boost::mutex& unblocked_count_mutex_,
                       boost::mutex& finish_mutex_):
            mutex(mutex_),
            unblocked_count(unblocked_count_),
            unblocked_count_mutex(unblocked_count_mutex_),
            finish_mutex(finish_mutex_)
        {}
        
        void operator()()
        {
            typename Mutex::scoped_lock lock(mutex);
            
            // we get here when unblocked
            increment_unblocked_count();
            // wait until we're allowed to finish
            boost::mutex::scoped_lock finish_lock(finish_mutex);
        }
    };

    unsigned read_sync_value(unsigned& value,boost::mutex& m)
    {
        boost::mutex::scoped_lock lock(m);
        return value;
    }
    
    template<typename Mutex>
    void test_threads_block_on_mutex()
    {
        unsigned unblocked_count=0;
        boost::mutex unblocked_count_mutex;
        boost::mutex finish_mutex;
        boost::mutex::scoped_lock finish_lock(finish_mutex);
        
        Mutex blocking_mutex;
        typename Mutex::scoped_lock blocking_lock(blocking_mutex);
        
        unsigned const number_of_threads=100;
        boost::thread_group pool;

        for (unsigned i=0; i < number_of_threads; ++i)
        {
            pool.create_thread( block_on_mutex<Mutex>(blocking_mutex,unblocked_count,unblocked_count_mutex,finish_mutex) );
        }

        boost::thread::sleep(delay(1));

        BOOST_CHECK(!read_sync_value(unblocked_count,unblocked_count_mutex));

        blocking_lock.unlock();

        boost::thread::sleep(delay(1));

        BOOST_CHECK(read_sync_value(unblocked_count,unblocked_count_mutex)==1);
        
        finish_lock.unlock();
    
        pool.join_all();
    
        BOOST_CHECK(read_sync_value(unblocked_count,unblocked_count_mutex)==number_of_threads);
    }
}


void test_lock_blocks_other_threads()
{
    test_threads_block_on_mutex<boost::mutex>();
    test_threads_block_on_mutex<boost::try_mutex>();
    test_threads_block_on_mutex<boost::timed_mutex>();
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: mutex test suite");

    test->add(BOOST_TEST_CASE(&test_mutex));
    test->add(BOOST_TEST_CASE(&test_try_mutex));
    test->add(BOOST_TEST_CASE(&test_timed_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_try_mutex));
    test->add(BOOST_TEST_CASE(&test_recursive_timed_mutex));
    test->add(BOOST_TEST_CASE(&test_loop_threads_on_mutex));
    test->add(BOOST_TEST_CASE(&test_lock_blocks_other_threads));

    return test;
}
