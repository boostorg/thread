// (C) Copyright 2008 Anthony Williams
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/unit_test.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>

void test_lock_two_uncontended()
{
    boost::mutex m1,m2;

    boost::mutex::scoped_lock l1(m1,boost::defer_lock),
        l2(m2,boost::defer_lock);

    BOOST_CHECK(!l1.owns_lock());
    BOOST_CHECK(!l2.owns_lock());
    
    boost::lock(l1,l2);
    
    BOOST_CHECK(l1.owns_lock());
    BOOST_CHECK(l2.owns_lock());
}

struct wait_data
{
    boost::mutex m;
    bool flag;
    boost::condition_variable cond;
    
    wait_data():
        flag(false)
    {}
    
    void wait()
    {
        boost::mutex::scoped_lock l(m);
        while(!flag)
        {
            cond.wait(l);
        }
    }

    template<typename Duration>
    bool timed_wait(Duration d)
    {
        boost::system_time const target=boost::get_system_time()+d;
        
        boost::mutex::scoped_lock l(m);
        while(!flag)
        {
            if(!cond.timed_wait(l,target))
            {
                return flag;
            }
        }
        return true;
    }
    
    void signal()
    {
        boost::mutex::scoped_lock l(m);
        flag=true;
        cond.notify_all();
    }
};
       

void lock_mutexes_slowly(boost::mutex* m1,boost::mutex* m2,wait_data* locked,wait_data* quit)
{
    boost::lock_guard<boost::mutex> l1(*m1);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    boost::lock_guard<boost::mutex> l2(*m2);
    locked->signal();
    quit->wait();
}

void lock_pair(boost::mutex* m1,boost::mutex* m2)
{
    boost::lock(*m1,*m2);
    boost::mutex::scoped_lock l1(*m1,boost::adopt_lock),
        l2(*m2,boost::adopt_lock);
}

void test_lock_two_other_thread_locks_in_order()
{
    boost::mutex m1,m2;
    wait_data locked;
    wait_data release;
    
    boost::thread t(lock_mutexes_slowly,&m1,&m2,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_pair,&m1,&m2);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(1)));

    release.signal();
    
    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(1)));

    t.join();
}

void test_lock_two_other_thread_locks_in_opposite_order()
{
    boost::mutex m1,m2;
    wait_data locked;
    wait_data release;
    
    boost::thread t(lock_mutexes_slowly,&m1,&m2,&locked,&release);
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    boost::thread t2(lock_pair,&m2,&m1);
    BOOST_CHECK(locked.timed_wait(boost::posix_time::seconds(1)));

    release.signal();
    
    BOOST_CHECK(t2.timed_join(boost::posix_time::seconds(1)));

    t.join();
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: generic locks test suite");

    test->add(BOOST_TEST_CASE(&test_lock_two_uncontended));
    test->add(BOOST_TEST_CASE(&test_lock_two_other_thread_locks_in_order));
    test->add(BOOST_TEST_CASE(&test_lock_two_other_thread_locks_in_opposite_order));

    return test;
}
