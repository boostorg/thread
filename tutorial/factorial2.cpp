#include <boost/thread/thread.hpp>
#include <boost/ref.hpp>
#include <iostream>

class factorial
{
public:
    factorial(int x) : x(x), res(0) { }
    void operator()() { res = calculate(x); }
    int result() const { return res; }

private:
    int calculate(int x) { return x <= 1 ? 1 : x * calculate(x-1); }

private:
    int x;
    int res;
};

int main()
{
    factorial f(10);
    boost::thread thrd(boost::ref(f));
    thrd.join();
    std::cout << "10! = " << f.result() << std::endl;
}