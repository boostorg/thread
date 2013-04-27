// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2013 Vicente J. Botet Escriba

#define BOOST_THREAD_PROVIDES_INTERRUPTIONS

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/completion_latch.hpp>

#include <boost/detail/lightweight_test.hpp>
#include <vector>

namespace
{

  // Shared variables for generation completion_latch test
  const int N_THREADS = 10;
  boost::completion_latch gen_latch(N_THREADS);
  boost::mutex mutex;
  long global_parameter;

  void latch_thread()
  {
    {
      boost::unique_lock<boost::mutex> lock(mutex);
      global_parameter++;
    }
    gen_latch.count_down();
    //do something else
  }

} // namespace

void test_global_parameter()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  BOOST_TEST_EQ(global_parameter, N_THREADS);
}

void reset_gen_latch()
{
  {
    boost::unique_lock<boost::mutex> lock(mutex);
    BOOST_TEST_EQ(global_parameter, N_THREADS);
  }
  gen_latch.reset(N_THREADS);
}

void test_completion_latch_reset()
{
  boost::thread_group g;
  boost::thread_group g2;

  gen_latch.then(&reset_gen_latch);

  {
    global_parameter = 0;
    try
    {
      for (int i = 0; i < N_THREADS; ++i)
        g.create_thread(&latch_thread);

      if (!gen_latch.try_wait())
        gen_latch.wait(); // All the threads have been updated the global_parameter
      //std::cout << __FILE__ << ':' << __LINE__ << std::endl;
      g.join_all();
    }
    catch (...)
    {
      g.interrupt_all();
      g.join_all();
      throw;
    }
  }
  //std::cout << __FILE__ << ':' << __LINE__ << std::endl;
  gen_latch.then(&test_global_parameter);
  {
    //std::cout << __FILE__ << ':' << __LINE__ << std::endl;
    global_parameter = 0;
    try
    {
      //std::cout << __FILE__ << ':' << __LINE__ << std::endl;
      for (int i = 0; i < N_THREADS; ++i)
        g2.create_thread(&latch_thread);

      //std::cout << __FILE__ << ':' << __LINE__ << std::endl;
      //if (!gen_latch.try_wait())
        gen_latch.wait(); // All the threads have been updated the global_parameter
      //std::cout << __FILE__ << ':' << __LINE__ << std::endl;

      g2.join_all();
    }
    catch (...)
    {
      g2.interrupt_all();
      g2.join_all();
      throw;
    }
  }
}
//template <bool b>
//struct xx {
//  static BOOST_CONSTEXPR_OR_CONST bool value = !b;
//};

int main()
{
  //test_completion_latch();
  test_completion_latch_reset();
  return boost::report_errors();

  //BOOST_CONSTEXPR_OR_CONST bool a = boost::integral_constant<bool,false>::value;
  //xx<a>
}

