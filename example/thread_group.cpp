#include <boost/thread/thread.hpp>
#include <iostream>

int count = 0;
boost::mutex mutex;

void increment_count()
{
   boost::mutex::scoped_lock lock(mutex);
   std::cout << "count = " << ++count << std::endl;
}

int main(int argc, char* argv[])
{
   boost::thread_group threads;
   for (int i = 0; i < 10; ++i)
      threads.create_thread(&increment_count);
   threads.join_all();
}
