#include <boost/thread.hpp>
#include <assert.h>
#include <iostream>
#include <stdlib.h>
#if defined(BOOST_THREAD_PLATFORM_PTHREAD)
#include <unistd.h>
#endif
boost::mutex mtx;
boost::condition_variable cv;

int main()
{
#if defined(BOOST_THREAD_PLATFORM_PTHREAD)

         for (int i=0; i<3; ++i) {
                 const time_t wait_time = ::time(0)+1;

                 boost::mutex::scoped_lock lk(mtx);
                 //const bool res =
                     (void)cv.timed_wait(lk, boost::posix_time::from_time_t(wait_time));
                 const time_t end_time = ::time(0);
                 std::cerr << "end_time=" <<  end_time << " \n";
                 std::cerr << "wait_time=" << wait_time << " \n";
                 std::cerr << end_time - wait_time << " \n";
                 assert(end_time >= wait_time);
                 std::cerr << " OK\n";
         }
#endif
         return 0;
}
