#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

boost::mutex mutex;
int counter=0;

void change_count()
{
	boost::mutex::scoped_lock lock(mutex);
    int i = ++counter;
    std::cout << "count == " << i << std::endl;
}

int main()
{
    const int num_threads = 4;
    boost::thread_group thrds;
    for (int i=0; i < num_threads; ++i)
        thrds.create_thread(&change_count);
    thrds.join_all();
}
