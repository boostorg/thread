#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

int main(int argc, char* argv[])
{
   boost::xtime xt;
   boost::xtime_get(&xt, boost::TIME_UTC);
   xt.sec += 1;
   boost::thread::sleep(xt); // Sleep for 1 second
}
