#include <vector>
#include <iostream>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>

namespace {
    const int ITERS = 100;
    boost::mutex io_mutex;
};

template <typename M>
class buffer_t : public M, public boost::condition
{
public:
    typedef typename M::lock lock;
    
    buffer_t(int n)
        : p(0), c(0), full(0), buf(n)
    {
    }
    
    void send(int m)
    {
        lock lk(*this);
        while (full == buf.size())
            wait(lk);
        buf[p] = m;
        p = (p+1) % buf.size();
        ++full;
        notify_all();
    }
    int receive()
    {
        lock lk(*this);
        while (full == 0)
            wait(lk);
        int i = buf[c];
        c = (c+1) % buf.size();
        --full;
        notify_all();
        return i;
    }
    
    static buffer_t& get_buffer()
    {
        static buffer_t buf(2);
        return buf;
    }
    
    static void do_sender_thread()
    {
        for (int n = 0; n < ITERS; ++n)
        {
            get_buffer().send(n);
            {
                volatile boost::mutex::lock lock(io_mutex);
                std::cout << "sent: " << n << std::endl;
            }
        }
    }
    
    static void do_receiver_thread()
    {
        int n;
        do
        {
            n = get_buffer().receive();
            {
                volatile boost::mutex::lock lock(io_mutex);
                std::cout << "received: " << n << std::endl;
            }
        } while (n < ITERS - 1);
    }
    
private:
    unsigned int p, c, full;
    std::vector<int> buf;
};

template <typename M>
void do_test(M* dummy=0)
{
    typedef buffer_t<M> buffer_type;
    buffer_type::get_buffer();
    boost::thread thrd1(&buffer_type::do_sender_thread);
    boost::thread thrd2(&buffer_type::do_receiver_thread);
    thrd1.join();
    thrd2.join();
}

void test_buffer()
{
    do_test<boost::mutex>();
    do_test<boost::recursive_mutex>();
}

int main()
{
    test_buffer();
    return 0;
}
