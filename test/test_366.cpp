#include "boost/thread/thread_guard.hpp"
#include "boost/thread.hpp"
#include <iostream>
#include <boost/atomic.hpp>

static boost::atomic_int g_sensor{0};
struct sensing_joiner : boost::interrupt_and_join_if_joinable {
  ~sensing_joiner() { ++g_sensor; }
};

static boost::atomic_int g_count_work{0};
static void inner_thread_func() {
  while (true) {
    boost::this_thread::interruption_point();
    boost::this_thread::no_interruption_point::sleep_for(
        boost::chrono::seconds(1));
    ++g_count_work;
  }
}

static void outer_thread_func() {
    boost::thread inner;
    boost::thread_guard<sensing_joiner> guard(inner);

    inner = boost::thread(inner_thread_func);
}

static void double_interrupt() {
  boost::thread outer(outer_thread_func);
  outer.interrupt();
  outer.join();
}

int main() {
  std::cout << "Start" << std::endl;
  double_interrupt();
  std::cout << "End" << std::endl;

  std::cout << "g_sum:    " << g_count_work << std::endl;
  std::cout << "g_sensor: " << g_sensor << std::endl;
  return 0;
}
