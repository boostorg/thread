#include <boost/thread/thread.hpp>
#include <boost/thread/once.hpp>
#include <boost/test/test_tools.hpp>

namespace {

int once_value = 0;
boost::once_flag once = BOOST_ONCE_INIT;

void init_once_value()
{
    once_value++;
}

void test_once_thread()
{
    boost::call_once(&init_once_value, once);
}

} // namespace

void test_call_once()
{
    const int NUMTHREADS=5;
    boost::thread_group threads;
    for (int i=0; i<NUMTHREADS; ++i)
        threads.create_thread(&test_once_thread);
    threads.join_all();
    BOOST_TEST(once_value == 1);
}
