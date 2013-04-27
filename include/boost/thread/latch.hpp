// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2013 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_LATCH_HPP
#define BOOST_THREAD_LATCH_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/assert.hpp>

//#include <boost/throw_exception.hpp>
//#include <stdexcept>
//#include <string>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  class latch
  {
    void wait(boost::unique_lock<boost::mutex> &lk)
    {
      while (count_ > 0)
      {
        cond_.wait(lk);
      }
    }
    /// Decrement the count and notify anyone waiting if we reach zero.
    /// @Requires count must be greater than 0
    /// @ThreadSafe
    bool count_down(unique_lock<mutex> &lk)
    {
      BOOST_ASSERT(count_ > 0);
      if (--count_ == 0)
      {
        cond_.notify_all();
        lk.unlock();
        return true;
      }
      return false;
    }

  public:
    BOOST_THREAD_NO_COPYABLE( latch )

    /// Constructs a latch with a given count.
    latch(std::size_t count) :
      count_(count)
    {
    }

    /// Blocks until the latch has counted down to zero.
    void wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      wait(lk);
    }

    /// @return true if the internal counter is already 0, false otherwise
    bool try_wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      return (count_ == 0);
    }

    /// try to wait for a specified amount of time is elapsed.
    /// @return whether there is a timeout or not.
    template <class Rep, class Period>
    cv_status wait_for(const chrono::duration<Rep, Period>& rel_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_ > 0)
      {
        return cond_.wait_for(rel_time);
      }
      else
      {
        return cv_status::no_timeout;
      }
    }

    /// try to wait until the specified time_point is reached
    /// @return whether there is a timeout or not.
    template <class lock_type, class Clock, class Duration>
    cv_status wait_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_ > 0)
      {
        return cond_.wait_until(abs_time);
      }
      else
      {
        return cv_status::no_timeout;
      }
    }

    /// Decrement the count and notify anyone waiting if we reach zero.
    /// @Requires count must be greater than 0
    void count_down()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      count_down(lk);
    }
    void signal() {count_down();}

    /// Decrement the count and notify anyone waiting if we reach zero.
    /// Blocks until the latch has counted down to zero.
    /// @Requires count must be greater than 0
    void count_down_and_wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_down(lk))
      {
        return;
      }
      wait(lk);
    }
    void sync() {count_down_and_wait();}

    /// Reset the counter
    /// #Requires This method may only be invoked when there are no other threads currently inside the count_down_and_wait() method.
    void reset(std::size_t count)
    {
      boost::lock_guard<boost::mutex> lk(mutex_);
      BOOST_ASSERT(count_ == 0);
      count_ = count;
    }

  private:
    mutex mutex_;
    condition_variable cond_;
    std::size_t count_;
  };

} // namespace boost

#include <boost/config/abi_suffix.hpp>

#endif
