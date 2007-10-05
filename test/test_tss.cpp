// Copyright (C) 2001-2003
// William E. Kempf
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/thread/tss.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <boost/test/unit_test.hpp>

#include <libs/thread/test/util.inl>

#include <iostream>

#if defined(BOOST_HAS_WINTHREADS)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>    
#endif

boost::mutex check_mutex;
boost::mutex tss_mutex;
int tss_instances = 0;
int tss_total = 0;

struct tss_value_t
{
    tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        ++tss_instances;
        ++tss_total;
        value = 0;
    }
    ~tss_value_t()
    {
        boost::mutex::scoped_lock lock(tss_mutex);
        --tss_instances;
    }
    int value;
};

boost::thread_specific_ptr<tss_value_t> tss_value;

void test_tss_thread()
{
    tss_value.reset(new tss_value_t());
    for (int i=0; i<1000; ++i)
    {
        int& n = tss_value->value;
        // Don't call BOOST_CHECK_EQUAL directly, as it doesn't appear to
        // be thread safe. Must evaluate further.
        if (n != i)
        {
            boost::mutex::scoped_lock lock(check_mutex);
            BOOST_CHECK_EQUAL(n, i);
        }
        ++n;
    }
}

#if defined(BOOST_HAS_WINTHREADS)
    typedef HANDLE native_thread_t;

    DWORD WINAPI test_tss_thread_native(LPVOID lpParameter)
    {
        test_tss_thread();
        return 0;
    }

    native_thread_t create_native_thread(void)
    {
        return CreateThread(
            0, //security attributes (0 = not inheritable)
            0, //stack size (0 = default) 
            &test_tss_thread_native, //function to execute
            0, //parameter to pass to function
            0, //creation flags (0 = run immediately)
            0  //thread id (0 = thread id not returned)
            );
    }

    void join_native_thread(native_thread_t thread)
    {
        DWORD res = WaitForSingleObject(thread, INFINITE);
        BOOST_CHECK(res == WAIT_OBJECT_0);

        res = CloseHandle(thread);
        BOOST_CHECK(SUCCEEDED(res));
    }
#endif

void do_test_tss()
{
    tss_instances = 0;
    tss_total = 0;

    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_tss_thread);
    threads.join_all();

    std::cout
        << "tss_instances = " << tss_instances
        << "; tss_total = " << tss_total
        << "\n";
    std::cout.flush();

    BOOST_CHECK_EQUAL(tss_instances, 0);
    BOOST_CHECK_EQUAL(tss_total, 5);

    #if defined(BOOST_HAS_WINTHREADS)
        tss_instances = 0;
        tss_total = 0;

        native_thread_t thread1 = create_native_thread();
        BOOST_CHECK(thread1 != 0);

        native_thread_t thread2 = create_native_thread();
        BOOST_CHECK(thread2 != 0);

        native_thread_t thread3 = create_native_thread();
        BOOST_CHECK(thread3 != 0);

        native_thread_t thread4 = create_native_thread();
        BOOST_CHECK(thread3 != 0);

        native_thread_t thread5 = create_native_thread();
        BOOST_CHECK(thread3 != 0);

        join_native_thread(thread5);
        join_native_thread(thread4);
        join_native_thread(thread3);
        join_native_thread(thread2);
        join_native_thread(thread1);

        std::cout
            << "tss_instances = " << tss_instances
            << "; tss_total = " << tss_total
            << "\n";
        std::cout.flush();

        // The following is not really an error. TSS cleanup support still is available for boost threads.
        // Also this usually will be triggered only when bound to the static version of thread lib.
        // 2006-10-02 Roland Schwarz
        //BOOST_CHECK_EQUAL(tss_instances, 0);
        BOOST_CHECK_MESSAGE(tss_instances == 0, "Support of automatic tss cleanup for native threading API not available");
        BOOST_CHECK_EQUAL(tss_total, 5);
    #endif
}

void test_tss()
{
    timed_test(&do_test_tss, 2);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: tss test suite");

    test->add(BOOST_TEST_CASE(test_tss));

    return test;
}
