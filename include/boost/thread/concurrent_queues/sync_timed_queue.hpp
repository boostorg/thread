// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_SYNC_TIMED_QUEUE_HPP
#define BOOST_THREAD_SYNC_TIMED_QUEUE_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>
#include <boost/chrono/time_point.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{
  template <class T>
  struct scheduled_type
  {
    typedef typename chrono::steady_clock clock;
    typedef chrono::steady_clock::time_point time_point;
    T data;
    time_point time;

    scheduled_type(T const&pdata, time_point tp) : data(pdata), time(tp) {}
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    scheduled_type(T &&pdata, time_point tp) : data(std::move(pdata)), time(tp) {}
#endif
    bool time_not_reached() const
    {
      return time > clock::now();
    }

    bool operator <(const scheduled_type<T> other) const
    {
      return this->time > other.time;
    }
  }; //end struct
} //end detail namespace

  template <class T>
  class sync_timed_queue
    : private sync_priority_queue<detail::scheduled_type<T> >
  {
    typedef detail::scheduled_type<T> stype;
    typedef sync_priority_queue<stype> super;
  public:
    //typedef typename stype::clock clock; // fixme
    typedef typename chrono::steady_clock clock;

    typedef typename clock::duration duration;
    typedef typename clock::time_point time_point;
    typedef T value_type;
    //typedef typename super::value_type value_type; // fixme
    typedef typename super::underlying_queue_type underlying_queue_type;
    typedef typename super::size_type size_type;
    typedef typename super::op_status op_status;

    sync_timed_queue() : super() {};
    ~sync_timed_queue() {}

    using super::size;
    using super::empty;
    using super::full;
    using super::close;
    using super::closed;

    T pull();
    optional<T> try_pull();
    optional<T> pull_no_wait();

    void push(const T& elem, const time_point& tp);
    void push(const T& elem, const duration& dura);
    queue_op_status try_push(const T& elem, const time_point& tp);
    queue_op_status try_push(const T& elem, const duration& dura);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    queue_op_status try_push(T&& elem, const time_point& tp);
    queue_op_status try_push(T&& elem, const duration& dura);
#endif
  private:
    T pull(unique_lock<mutex>&);
    T pull(lock_guard<mutex>&);
    T pull_when_time_reached(unique_lock<mutex>&);
    bool time_not_reached(unique_lock<mutex>&);
    bool time_not_reached(lock_guard<mutex>&);

    sync_timed_queue(const sync_timed_queue&);
    sync_timed_queue& operator=(const sync_timed_queue&);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    sync_timed_queue(sync_timed_queue&&);
    sync_timed_queue& operator=(sync_timed_queue&&);
#endif
  }; //end class

  template <class T>
  void sync_timed_queue<T>::push(const T& elem, const time_point& tp)
  {
    super::push(stype(elem,tp));
  }

  template <class T>
  void sync_timed_queue<T>::push(const T& elem, const duration& dura)
  {
    push(elem, clock::now() + dura);
  }

  template <class T>
  queue_op_status sync_timed_queue<T>::try_push(const T& elem, const time_point& tp)
  {
    return super::try_push(stype(elem,tp));
  }

  template <class T>
  queue_op_status sync_timed_queue<T>::try_push(const T& elem, const duration& dura)
  {
    return try_push(elem,clock::now() + dura);
  }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
  template <class T>
  queue_op_status sync_timed_queue<T>::try_push(T&& elem, const time_point& tp)
  {
    return super::try_push(stype(std::forward<T>(elem), tp));
  }

  template <class T>
  queue_op_status sync_timed_queue<T>::try_push(T&& elem, const duration& dura)
  {
    return try_push(std::forward<T>(elem), clock::now() + dura);
  }

#endif

  template <class T>
  bool sync_timed_queue<T>::time_not_reached(unique_lock<mutex>&)
  {
    return super::data_.top().time_not_reached();
  }

  template <class T>
  bool sync_timed_queue<T>::time_not_reached(lock_guard<mutex>&)
  {
    return super::data_.top().time_not_reached();
  }

  template <class T>
  T sync_timed_queue<T>::pull(unique_lock<mutex>&)
  {
    const T temp = super::data_.top().data;
    super::data_.pop();
    return temp;
  }

  template <class T>
  T sync_timed_queue<T>::pull(lock_guard<mutex>&)
  {
    const T temp = super::data_.top().data;
    super::data_.pop();
    return temp;
  }

  template <class T>
  T sync_timed_queue<T>::pull_when_time_reached(unique_lock<mutex>& lk)
  {
    while (time_not_reached(lk))
    {
      super::not_empty_.wait_until(lk,super::data_.top().time);
      super::wait_until_not_empty(lk);
    }
    return pull(lk);
  }

  template <class T>
  T sync_timed_queue<T>::pull()
  {
    unique_lock<mutex> lk(super::mtx_);
    super::wait_until_not_empty(lk);
    return pull_when_time_reached(lk);
  }

  template <class T>
  optional<T> sync_timed_queue<T>::try_pull()
  {
    unique_lock<mutex> lk(super::mtx_);
    if (! lk.owns_lock()) return optional<T>();
    super::wait_until_not_empty(lk);
    return make_optional( pull_when_time_reached(lk) );
  }

  template <class T>
  optional<T> sync_timed_queue<T>::pull_no_wait()
  {
    lock_guard<mutex> lk(super::mtx_);
    if ( super::data_.empty() ) return optional<T>();
    if ( time_not_reached(lk) ) return optional<T>();
    return make_optional( pull(lk) );
  }

} //end concurrent namespace

using concurrent::sync_timed_queue;

} //end boost namespace
#include <boost/config/abi_suffix.hpp>

#endif
