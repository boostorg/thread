#include <boost/thread/thread.hpp>
#include <iostream>

struct helloworld
{
    helloworld() { }
    void operator()() { std::cout << "Hello World." << std::endl; }
};

int main()
{
    boost::thread thrd(helloworld());
    thrd.join();
}