#include <boost/thread/thread.hpp>
#include <iostream>

struct helloworld
{
    helloworld(const char* who) : m_who(who) { }
    void operator()() { std::cout << m_who << "says, \"Hello World.\"" << std::endl; }
    const char* m_who;
};

int main()
{
    boost::thread thrd(helloworld("Bob"));
    thrd.join();
}