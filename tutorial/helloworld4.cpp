#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <iostream>

void helloworld(const char* who)
{
    std::cout << who << "says, \"Hello World.\"" << std::endl;
}

int main()
{
    boost::thread thrd(boost::bind(&helloworld, "Bob"));
    thrd.join();
}