#include <boost/thread/thread.hpp>
#include <iostream>

void helloworld()
{
    std::cout << "Hello World!" << std::endl;
}

int main()
{
    boost::thread thrd(&helloworld);
    thrd.join();
}