#include <boost/thread/thread.hpp>
#include <iostream>

class factorial
{
public:
    factorial(int x, int& res) : x(x), res(res) { }
    void operator()() { res = calculate(x); }
    int result() const { return res; }

private:
    int calculate(int x) { return x <= 1 ? 1 : x * calculate(x-1); }

private:
    int x;
    int& res;
};

int main()
{
    int result;
    factorial f(10, result);
    boost::thread thrd(f);
    thrd.join();
    std::cout << "10! = " << result << std::endl;
}