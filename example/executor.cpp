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

void push(boost::csbl::deque<boost::thread_detail::work> &data_, BOOST_THREAD_RV_REF(boost::thread_detail::work) closure)
{
  try
  {
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
    boost::thread_detail::work  v;
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
    //v = boost::move(closure);
    //v = boost::forward<boost::thread_detail::work>(closure);
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;

    data_.push_back(boost::move(closure));
    BOOST_THREAD_LOG
      << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;

    //data_.push_back(boost::forward<boost::thread_detail::work>(closure));
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
void submit(boost::csbl::deque<boost::thread_detail::work> &data_, BOOST_THREAD_FWD_REF(Closure) closure)
{
  BOOST_THREAD_LOG
    << boost::this_thread::get_id()  << " <MAIN" << BOOST_THREAD_END_LOG;
  //work w =boost::move(closure);
  //work_queue.push(boost::move(w));
  //push(data_, boost::thread_detail::work(boost::forward<Closure>(closure)));
  boost::thread_detail::work  v =boost::forward<Closure>(closure);
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
      boost::thread_detail::work f(&p1);

    boost::csbl::deque<boost::thread_detail::work> data_;
    data_.push_back(boost::move(f));
    data_.push_back(boost::thread_detail::work(&p1));
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

    typedef boost::csbl::vector<boost::thread> thread_vector;
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
