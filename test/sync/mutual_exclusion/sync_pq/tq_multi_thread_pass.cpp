// Copyright (C) 2019 Austin Beer
// Copyright (C) 2019 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_PROVIDES_EXECUTORS

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/concurrent_queues/sync_timed_queue.hpp>

#include <boost/core/lightweight_test.hpp>
#include "../../../timming.hpp"

using namespace boost::chrono;

typedef boost::concurrent::sync_timed_queue<int> sync_tq;

const int count = 5;

void call_push(sync_tq* q)
{
    // push elements onto the queue every 500 milliseconds but with a decreasing delay each time
    for (int i = 0; i < count; ++i)
    {
        q->push(i, sync_tq::clock::now() + seconds(count - i));
        boost::this_thread::sleep_for(milliseconds(500));
    }
}

void call_pull(sync_tq* q)
{
    // pull elements off of the queue (earliest element first)
    steady_clock::time_point start = steady_clock::now();
    for (int i = count - 1; i >= 0; --i)
    {
        int j;
        q->pull(j);
        BOOST_TEST_EQ(i, j);
        milliseconds elapsed = duration_cast<milliseconds>(steady_clock::now() - start);
        milliseconds expected = milliseconds(i * 500) + seconds(count - i);
        BOOST_TEST_GE(elapsed, expected - milliseconds(BOOST_THREAD_TEST_TIME_MS));
        BOOST_TEST_LE(elapsed, expected + milliseconds(BOOST_THREAD_TEST_TIME_MS));
    }
}

void test_push_while_pull()
{
    sync_tq tq;
    BOOST_TEST(tq.empty());
    boost::thread_group tg;
    tg.create_thread(boost::bind(call_push, &tq));
    tg.create_thread(boost::bind(call_pull, &tq));
    tg.join_all();
    BOOST_TEST(tq.empty());
}

int main()
{
    test_push_while_pull();
    return boost::report_errors();
}
