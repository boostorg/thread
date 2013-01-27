#ifndef BOOST_THREAD_V2_SHARED_MUTEX_HPP
#define BOOST_THREAD_V2_SHARED_MUTEX_HPP

//  shared_mutex.hpp
//
// Copyright Howard Hinnant 2007-2010.
// Copyright Vicente J. Botet Escriba 2012.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/*
<shared_mutex> synopsis

namespace boost
{
namespace thread_v2
{

class shared_mutex
{
public:

    shared_mutex();
    ~shared_mutex();

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    // Exclusive ownership

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    template <class Rep, class Period>
        bool
        try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared();
};

class upgrade_mutex
{
public:

    upgrade_mutex();
    ~upgrade_mutex();

    upgrade_mutex(const upgrade_mutex&) = delete;
    upgrade_mutex& operator=(const upgrade_mutex&) = delete;

    // Exclusive ownership

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    template <class Rep, class Period>
        bool
        try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared();

    // Upgrade ownership

    void lock_upgrade();
    bool try_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_lock_upgrade_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_upgrade_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade();

    // Shared <-> Exclusive

    bool try_unlock_shared_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_shared();

    // Shared <-> Upgrade

    bool try_unlock_shared_and_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_upgrade_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_upgrade_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade_and_lock_shared();

    // Upgrade <-> Exclusive

    void unlock_upgrade_and_lock();
    bool try_unlock_upgrade_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_upgrade_and_lock_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_upgrade_and_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_upgrade();
};

}  // thread_v2
}  // boost

 */

#include <boost/thread/detail/config.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include <climits>
#include <boost/system/system_error.hpp>

namespace boost {
  namespace thread_v2 {

    class shared_mutex
    {
      typedef ::boost::mutex              mutex_t;
      typedef ::boost::condition_variable cond_t;
      typedef unsigned                count_t;

      mutex_t mut_;
      cond_t  gate1_;
      cond_t  gate2_;
      count_t state_;

      static const count_t write_entered_ = 1U << (sizeof(count_t)*CHAR_BIT - 1);
      static const count_t n_readers_ = ~write_entered_;

    public:
      shared_mutex();
      ~shared_mutex();

#ifndef BOOST_NO_DELETED_FUNCTIONS
      shared_mutex(shared_mutex const&) = delete;
      shared_mutex& operator=(shared_mutex const&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
      shared_mutex(shared_mutex const&);
      shared_mutex& operator=(shared_mutex const&);
    public:
#endif // BOOST_NO_DELETED_FUNCTIONS

      // Exclusive ownership

      void lock();
      bool try_lock();
      template <class Rep, class Period>
      bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_until(boost::chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock();

      // Shared ownership

      void lock_shared();
      bool try_lock_shared();
      template <class Rep, class Period>
      bool
      try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_shared_until(boost::chrono::steady_clock::now() +
            rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_lock_shared_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_shared();
    };

    template <class Clock, class Duration>
    bool
    shared_mutex::try_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (state_ & write_entered_)
      {
        while (true)
        {
          boost::cv_status status = gate1_.wait_until(lk, abs_time);
          if ((state_ & write_entered_) == 0)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      state_ |= write_entered_;
      if (state_ & n_readers_)
      {
        while (true)
        {
          boost::cv_status status = gate2_.wait_until(lk, abs_time);
          if ((state_ & n_readers_) == 0)
            break;
          if (status == boost::cv_status::timeout)
          {
            state_ &= ~write_entered_;
            return false;
          }
        }
      }
      return true;
    }

    template <class Clock, class Duration>
    bool
    shared_mutex::try_lock_shared_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if ((state_ & write_entered_) || (state_ & n_readers_) == n_readers_)
      {
        while (true)
        {
          boost::cv_status status = gate1_.wait_until(lk, abs_time);
          if ((state_ & write_entered_) == 0 &&
              (state_ & n_readers_) < n_readers_)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }

    class upgrade_mutex
    {
      typedef boost::mutex              mutex_t;
      typedef boost::condition_variable cond_t;
      typedef unsigned                count_t;

      mutex_t mut_;
      cond_t  gate1_;
      cond_t  gate2_;
      count_t state_;

      static const unsigned write_entered_ = 1U << (sizeof(count_t)*CHAR_BIT - 1);
      static const unsigned upgradable_entered_ = write_entered_ >> 1;
      static const unsigned n_readers_ = ~(write_entered_ | upgradable_entered_);

    public:

      upgrade_mutex();
      ~upgrade_mutex();

#ifndef BOOST_NO_DELETED_FUNCTIONS
      upgrade_mutex(const upgrade_mutex&) = delete;
      upgrade_mutex& operator=(const upgrade_mutex&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
      upgrade_mutex(const upgrade_mutex&);
      upgrade_mutex& operator=(const upgrade_mutex&);
    public:
#endif // BOOST_NO_DELETED_FUNCTIONS

      // Exclusive ownership

      void lock();
      bool try_lock();
      template <class Rep, class Period>
      bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_until(boost::chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock();

      // Shared ownership

      void lock_shared();
      bool try_lock_shared();
      template <class Rep, class Period>
      bool
      try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_shared_until(boost::chrono::steady_clock::now() +
            rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_lock_shared_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_shared();

      // Upgrade ownership

      void lock_upgrade();
      bool try_lock_upgrade();
      template <class Rep, class Period>
      bool
      try_lock_upgrade_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_upgrade_until(boost::chrono::steady_clock::now() +
            rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_lock_upgrade_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_upgrade();

      // Shared <-> Exclusive

      bool try_unlock_shared_and_lock();
      template <class Rep, class Period>
      bool
      try_unlock_shared_and_lock_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_shared_and_lock_until(
            boost::chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_unlock_shared_and_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_and_lock_shared();

      // Shared <-> Upgrade

      bool try_unlock_shared_and_lock_upgrade();
      template <class Rep, class Period>
      bool
      try_unlock_shared_and_lock_upgrade_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_shared_and_lock_upgrade_until(
            boost::chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_unlock_shared_and_lock_upgrade_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_upgrade_and_lock_shared();

      // Upgrade <-> Exclusive

      void unlock_upgrade_and_lock();
      bool try_unlock_upgrade_and_lock();
      template <class Rep, class Period>
      bool
      try_unlock_upgrade_and_lock_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_upgrade_and_lock_until(
            boost::chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool
      try_unlock_upgrade_and_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
      void unlock_and_lock_upgrade();
    };

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (state_ & (write_entered_ | upgradable_entered_))
      {
        while (true)
        {
          boost::cv_status status = gate1_.wait_until(lk, abs_time);
          if ((state_ & (write_entered_ | upgradable_entered_)) == 0)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      state_ |= write_entered_;
      if (state_ & n_readers_)
      {
        while (true)
        {
          boost::cv_status status = gate2_.wait_until(lk, abs_time);
          if ((state_ & n_readers_) == 0)
            break;
          if (status == boost::cv_status::timeout)
          {
            state_ &= ~write_entered_;
            return false;
          }
        }
      }
      return true;
    }

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_lock_shared_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if ((state_ & write_entered_) || (state_ & n_readers_) == n_readers_)
      {
        while (true)
        {
          boost::cv_status status = gate1_.wait_until(lk, abs_time);
          if ((state_ & write_entered_) == 0 &&
              (state_ & n_readers_) < n_readers_)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_lock_upgrade_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if ((state_ & (write_entered_ | upgradable_entered_)) ||
          (state_ & n_readers_) == n_readers_)
      {
        while (true)
        {
          boost::cv_status status = gate1_.wait_until(lk, abs_time);
          if ((state_ & (write_entered_ | upgradable_entered_)) == 0 &&
              (state_ & n_readers_) < n_readers_)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= upgradable_entered_ | num_readers;
      return true;
    }

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_unlock_shared_and_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (state_ != 1)
      {
        while (true)
        {
          boost::cv_status status = gate2_.wait_until(lk, abs_time);
          if (state_ == 1)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      state_ = write_entered_;
      return true;
    }

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_unlock_shared_and_lock_upgrade_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if ((state_ & (write_entered_ | upgradable_entered_)) != 0)
      {
        while (true)
        {
          boost::cv_status status = gate2_.wait_until(lk, abs_time);
          if ((state_ & (write_entered_ | upgradable_entered_)) == 0)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      state_ |= upgradable_entered_;
      return true;
    }

    template <class Clock, class Duration>
    bool
    upgrade_mutex::try_unlock_upgrade_and_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if ((state_ & n_readers_) != 1)
      {
        while (true)
        {
          boost::cv_status status = gate2_.wait_until(lk, abs_time);
          if ((state_ & n_readers_) == 1)
            break;
          if (status == boost::cv_status::timeout)
            return false;
        }
      }
      state_ = write_entered_;
      return true;
    }

  }  // thread_v2
}  // boost

namespace boost {
  //using thread_v2::shared_mutex;
  using thread_v2::upgrade_mutex;
  typedef thread_v2::upgrade_mutex shared_mutex;
}

#endif
