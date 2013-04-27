// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2013 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_COMPLETION_LATCH_HPP
#define BOOST_THREAD_COMPLETION_LATCH_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/assert.hpp>
#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
#include <boost/function.hpp>
#else
#include <functional>
#endif
#include <boost/thread/latch.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace thread_detail
  {
    void noop()
    {
    }
  }
  class completion_latch
  {
  public:
    /// the implementation defined completion function type
#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
    typedef function<void()> completion_function;
#else
    typedef std::function<void()> completion_function;
#endif
    /// noop completion function factory
    static completion_function noop()
    {
      return completion_function (&thread_detail::noop);
    }

  private:

    void wait_for_no_leaver(unique_lock<mutex> &lk)
    {
      // wait until all preceding waiting threads have leave
      while (leavers_ > 0)
      {
        idle_.wait(lk);
      }
    }
    void wait_for_leavers(unique_lock<mutex> &lk)
    {
      while (leavers_ == 0)
      {
        idle_.wait(lk);
      }
    }
    void inc_waiters(boost::unique_lock<boost::mutex> &)
    {
      ++waiters_;
      waiters_cnd_.notify_all();
    }
    void dec_waiters(boost::unique_lock<boost::mutex> &)
    {
      --waiters_;
      waiters_cnd_.notify_all();
    }
    void pre_wait(boost::unique_lock<boost::mutex> &lk)
    {
      wait_for_no_leaver(lk);
      inc_waiters(lk);
      wait_for_leavers(lk);
    }
    void post_wait(boost::unique_lock<boost::mutex> &lk)
    {
      dec_waiters(lk);
    }
    void wait(boost::unique_lock<boost::mutex> &lk)
    {
      pre_wait(lk);
      while (count_ > 0)
      {
        count_cond_.wait(lk);
      }
      post_wait(lk);
    }

    void wait_for_waiters(unique_lock<mutex> &lk)
    {
      // waits at least for a waiter.
      while (waiters_ == 0)
      {
        waiters_cnd_.wait(lk);
      }
    }
    void set_leavers()
    {
      leavers_ = waiters_;
      idle_.notify_all();
    }
    void wait_for_no_waiter(unique_lock<mutex> &lk)
    {
      while (waiters_ > 0)
        waiters_cnd_.wait(lk);
    }
    void reset_waiters_and_readers()
    {
      waiters_ = 0;
      leavers_ = 0;
      idle_.notify_all();
    }
    bool count_down(unique_lock<mutex> &lk)
    {
      BOOST_ASSERT(count_ > 0);
      if (--count_ == 0)
      {
        wait_for_waiters(lk);
        set_leavers();
        count_cond_.notify_all();
        wait_for_no_waiter(lk);
        reset_waiters_and_readers();
        lk.unlock();
        funct_();
        return true;
      }
      return false;
    }

  public:
    BOOST_THREAD_NO_COPYABLE( completion_latch)

    /// Constructs a latch with a given count.
    completion_latch(std::size_t count) :
      count_(count), funct_(noop()), waiters_(0), leavers_(0)
    {
    }

    /// Constructs a latch with a given count and a completion function.
    template <typename F>
    completion_latch(std::size_t count, BOOST_THREAD_RV_REF(F) funct) :
    count_(count),
    funct_(boost::move(funct)),
    waiters_(0),
    leavers_(0)
    {
    }
    template <typename F>
    completion_latch(std::size_t count, void(*funct)()) :
      count_(count), funct_(funct), waiters_(0), leavers_(0)
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
      pre_wait(lk);
      bool res = (count_ == 0);
      post_wait(lk);
      return res;
    }

    /// try to wait for a specified amount of time
    /// @return whether there is a timeout or not.
    template <class Rep, class Period>
    cv_status wait_for(const chrono::duration<Rep, Period>& rel_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      pre_wait(lk);
      cv_status res;
      if (count_ > 0)
      {
        res = count_cond_.wait_for(rel_time);
      }
      else
      {
        res = cv_status::no_timeout;
      }
      post_wait(lk);
      return res;
    }

    /// try to wait until the specified time_point is reached
    /// @return whether there is a timeout or not.
    template <class lock_type, class Clock, class Duration>
    cv_status wait_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      pre_wait(lk);
      cv_status res;
      if (count_ > 0)
      {
        res = count_cond_.wait_until(abs_time);
      }
      else
      {
        res = cv_status::no_timeout;
      }
      post_wait(lk);
      return res;
    }

    /// Decrement the count and notify anyone waiting if we reach zero.
    /// @Requires count must be greater than 0
    void count_down()
    {
      unique_lock<mutex> lk(mutex_);
      count_down(lk);
    }
    void signal()
    {
      count_down();
    }

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
    void sync()
    {
      count_down_and_wait();
    }

    /// Reset the counter
    /// #Requires This method may only be invoked when there are no other threads currently inside the count_down_and_wait() method.
    void reset(std::size_t count)
    {
      boost::lock_guard<boost::mutex> lk(mutex_);
      BOOST_ASSERT(count_ == 0);
      count_ = count;
    }

    /// Resets the latch with the new completion function.
    /// The next time the internal count reaches 0, this function will be invoked.
    /// This completion function may only be invoked when there are no other threads
    /// currently inside the count_down and wait related functions.
    /// It may also be invoked from within the registered completion function.
    /// @Returns the old completion function if any or noop if

#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
        template <typename F>
        completion_function then(BOOST_THREAD_RV_REF(F) funct)
        {
          boost::lock_guard<boost::mutex> lk(mutex_);
          completion_function tmp(funct_);
          funct_ = boost::move(funct);
          return tmp;
        }
#endif
    completion_function then(void(*funct)())
    {
      boost::lock_guard<boost::mutex> lk(mutex_);
      completion_function tmp(funct_);
      funct_ = completion_function(funct);
      return tmp;
    }

  private:
    mutex mutex_;
    condition_variable count_cond_;
    std::size_t count_;
    completion_function funct_;
    condition_variable waiters_cnd_;
    std::size_t waiters_;
    condition_variable idle_;
    std::size_t leavers_;
  };

} // namespace boost

#include <boost/config/abi_suffix.hpp>

#endif
