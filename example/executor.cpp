// Copyright (C) 2012-2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_THREAD_VERSION 4
#define BOOST_THREAD_USES_LOG
#define BOOST_THREAD_USES_LOG_THREAD_ID

#include <boost/thread/detail/log.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/thread/executor.hpp>
#include <boost/assert.hpp>
#include <string>

void p1()
{
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " P1" << BOOST_THREAD_END_LOG;
}

void p2()
{
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " P2" << BOOST_THREAD_END_LOG;
}

void push(boost::container::deque<boost::detail::function_wrapper> &data_, BOOST_THREAD_RV_REF(boost::detail::function_wrapper) closure)
{
  try
  {
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
    boost::detail::function_wrapper  v;
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
    //v = boost::move(closure);
    //v = boost::forward<boost::detail::function_wrapper>(closure);
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;

    data_.push_back(boost::move(closure));
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;

    //data_.push_back(boost::forward<boost::detail::function_wrapper>(closure));
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;

  }
  catch (std::exception& ex)
  {
    BOOST_THREAD_LOG
      << "ERRORRRRR " << ex.what() << "" << BOOST_THREAD_END_LOG;
  }
  catch (...)
  {
    BOOST_THREAD_LOG
      << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
  }
}

template <typename Closure>
void submit(boost::container::deque<boost::detail::function_wrapper> &data_, BOOST_THREAD_FWD_REF(Closure) closure)
{
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
  //work w =boost::move(closure);
  //work_queue.push(boost::move(w));
  //push(data_, boost::detail::function_wrapper(boost::forward<Closure>(closure)));
  boost::detail::function_wrapper  v =boost::forward<Closure>(closure);
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
  push(data_, boost::move(v));

  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
}

int main()
{
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
#if 0
  {
    try
    {
      boost::detail::function_wrapper f(&p1);

    boost::container::deque<boost::detail::function_wrapper> data_;
    data_.push_back(boost::move(f));
    data_.push_back(boost::detail::function_wrapper(&p1));
    submit(data_, &p1);
    }
    catch (std::exception& ex)
    {
      BOOST_THREAD_LOG
        << "ERRORRRRR " << ex.what() << "" << BOOST_THREAD_END_LOG;
    }
    catch (...)
    {
      BOOST_THREAD_LOG
        << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
    }

    typedef boost::container::vector<boost::thread> thread_vector;
    thread_vector threads;

  }
#endif
#if 1
  {
    try
    {
      boost::executor_adaptor<boost::thread_pool> ea;
      boost::executor &tp=ea;
      BOOST_THREAD_LOG
        << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
      tp.submit(&p1);
      BOOST_THREAD_LOG
        << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
      tp.submit(&p2);
      tp.submit(&p1);
      tp.submit(&p2);
      tp.submit(&p1);
      tp.submit(&p2);
      tp.submit(&p1);
      tp.submit(&p2);
      tp.submit(&p1);
      tp.submit(&p2);
    }
    catch (std::exception& ex)
    {
      BOOST_THREAD_LOG
        << "ERRORRRRR " << ex.what() << "" << BOOST_THREAD_END_LOG;
      return 1;
    }
    catch (...)
    {
      BOOST_THREAD_LOG
        << " ERRORRRRR exception thrown" << BOOST_THREAD_END_LOG;
      return 2;
    }
  }
#endif
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << "MAIN>" << BOOST_THREAD_END_LOG;
  return 0;
}
