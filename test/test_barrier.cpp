// test_barrier.cpp
//
// A simple test for a barrier implementation.

#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <iostream>

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

const int N_THREADS=10;

// Shared variables for generation barrier test
boost::barrier gen_barrier(N_THREADS);

boost::mutex mutex;

long global_parameter;

void worker()
{
    for (int i = 0; i < 5; ++i)
    {
        if (gen_barrier.wait())
		{
			boost::mutex::scoped_lock lock(mutex);
            global_parameter++;
		}
    }
}

void test_barrier()
{
    boost::thread_group g;
    global_parameter = 0;

    for (int i = 0; i < N_THREADS; ++i)
        g.create_thread(&worker);

    g.join_all();

    BOOST_TEST(global_parameter == 5);
}

int test_main(int argc, char *argv[])
{
    test_barrier();
    return 0;
}
