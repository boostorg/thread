#include <boost/thread/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/test/test_tools.hpp>

namespace {

// Shared variables for generation barrier test
const int N_THREADS=10;
boost::barrier gen_barrier(N_THREADS);
boost::mutex mutex;
long global_parameter;

void barrier_thread()
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

} // namespace 

void test_barrier()
{
    boost::thread_group g;
    global_parameter = 0;

    for (int i = 0; i < N_THREADS; ++i)
        g.create_thread(&barrier_thread);

    g.join_all();

    BOOST_TEST(global_parameter == 5);
}

