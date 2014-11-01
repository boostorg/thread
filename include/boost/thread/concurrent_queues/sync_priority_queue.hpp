// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_SYNC_PRIORITY_QUEUE
#define BOOST_THREAD_SYNC_PRIORITY_QUEUE

#include <boost/thread/detail/config.hpp>

#include <boost/thread/concurrent_queues/detail/sync_queue_base.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/csbl/vector.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/atomic.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/optional.hpp>

#include <exception>
#include <queue>
#include <utility>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
  template <class ValueType,
            class Container = csbl::vector<ValueType>,
            class Compare = std::less<typename Container::value_type> >
  class sync_priority_queue
    : public detail::sync_queue_base<ValueType, std::priority_queue<ValueType,Container,Compare> >
  {
    typedef detail::sync_queue_base<ValueType, std::priority_queue<ValueType,Container,Compare> >  super;
  public:
    typedef ValueType value_type;
    //typedef typename super::value_type value_type; // fixme
    typedef typename super::underlying_queue_type underlying_queue_type;
    typedef typename super::size_type size_type;
    typedef typename super::op_status op_status;

    typedef chrono::steady_clock clock;
  protected:

  public:
    sync_priority_queue() {}

    ~sync_priority_queue()
    {
      if(!super::closed())
      {
        super::close();
      }
    }

    void push(const ValueType& elem);
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    bool try_push(const ValueType& elem);
#else
    queue_op_status try_push(const ValueType& elem);
#endif

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void push(ValueType&& elem);
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    bool try_push(ValueType&& elem);
#else
    queue_op_status try_push(ValueType&& elem);
#endif
#endif

    ValueType pull();
    optional<ValueType> pull_until(const clock::time_point&);
    optional<ValueType> pull_for(const clock::duration&);
    optional<ValueType> pull_no_wait();

    optional<ValueType> try_pull();
    optional<ValueType> try_pull_no_wait();

  private:
    void push(unique_lock<mutex>&, const ValueType& elem);
    void push(lock_guard<mutex>&, const ValueType& elem);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    void push(unique_lock<mutex>&, ValueType&& elem);
    void push(lock_guard<mutex>&, ValueType&& elem);
#endif

    ValueType pull(unique_lock<mutex>&);
    ValueType pull(lock_guard<mutex>&);

    sync_priority_queue(const sync_priority_queue&);
    sync_priority_queue& operator= (const sync_priority_queue&);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    sync_priority_queue(sync_priority_queue&&);
    sync_priority_queue& operator= (sync_priority_queue&&);
#endif
  }; //end class


  template <class T,class Container, class Cmp>
  T sync_priority_queue<T,Container,Cmp>::pull(unique_lock<mutex>&)
  {
    T first = super::data_.top();
    super::data_.pop();
    return first;
  }
  template <class T,class Container, class Cmp>
  T sync_priority_queue<T,Container,Cmp>::pull(lock_guard<mutex>&)
  {
    T first = super::data_.top();
    super::data_.pop();
    return first;
  }

  template <class T,class Container, class Cmp>
  T sync_priority_queue<T,Container,Cmp>::pull()
  {
    unique_lock<mutex> lk(super::mtx_);
    super::wait_until_not_empty(lk);
    return pull(lk);
  }

  template <class T, class Cont,class Cmp>
  optional<T>
  sync_priority_queue<T,Cont,Cmp>::pull_until(const clock::time_point& tp)
  {
    unique_lock<mutex> lk(super::mtx_);
    while(super::data_.empty())
    {
      if(super::closed(lk)) throw std::exception();
      if(super::not_empty_.wait_until(lk, tp) == cv_status::timeout ) return optional<T>();
    }
    return make_optional( pull(lk) );
  }

  template <class T, class Cont,class Cmp>
  optional<T>
  sync_priority_queue<T,Cont,Cmp>::pull_for(const clock::duration& dura)
  {
    return pull_until(clock::now() + dura);
  }

  template <class T, class Container,class Cmp>
  optional<T>
  sync_priority_queue<T,Container,Cmp>::pull_no_wait()
  {
    lock_guard<mutex> lk(super::mtx_);
    if (super::data_.empty()) return optional<T>();
    return make_optional( pull(lk) );
  }

  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(unique_lock<mutex>&, const T& elem)
  {
    super::data_.push(elem);
    super::not_empty_.notify_one();
  }
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(lock_guard<mutex>&, const T& elem)
  {
    super::data_.push(elem);
    super::not_empty_.notify_one();
  }
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(const T& elem)
  {
    lock_guard<mutex> lk(super::mtx_);
    push(lk, elem);
  }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(unique_lock<mutex>&, T&& elem)
  {
    super::data_.emplace(elem);
    super::not_empty_.notify_one();
  }
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(lock_guard<mutex>&, T&& elem)
  {
    super::data_.emplace(elem);
    super::not_empty_.notify_one();
  }
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(T&& elem)
  {
    lock_guard<mutex> lk(super::mtx_);
    push(lk, std::forward<T>(elem));
  }
#endif

  template <class T, class Container,class Cmp>
  optional<T>
  sync_priority_queue<T,Container,Cmp>::try_pull()
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (! lk.owns_lock() ) return optional<T>();
    super::wait_until_not_empty(lk);
    return make_optional( pull(lk) );
  }

  template <class T, class Container,class Cmp>
  optional<T>
  sync_priority_queue<T,Container,Cmp>::try_pull_no_wait()
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if(! lk.owns_lock()  || super::data_.empty()) return optional<T>();
    return make_optional( pull(lk) );
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <class T, class Container,class Cmp>
  bool sync_priority_queue<T,Container,Cmp>::try_push(const T& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (! lk.owns_lock() ) return false;
    push(lk, elem);
    return true;
  }
#else
  template <class T, class Container,class Cmp>
  queue_op_status sync_priority_queue<T,Container,Cmp>::try_push(const T& elem)
  {
    lock_guard<mutex> lk(super::mtx_);
    if (super::closed(lk)) return queue_op_status::closed;
    push(lk, elem);
    return queue_op_status::success;
  }
#endif

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <class T, class Container,class Cmp>
  bool sync_priority_queue<T,Container,Cmp>::try_push(T&& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (! lk.owns_lock()) return false;
    push(lk, std::forward<T>(elem));
    return true;
  }
#else
  template <class T, class Container,class Cmp>
  queue_op_status sync_priority_queue<T,Container,Cmp>::try_push(T&& elem)
  {
    lock_guard<mutex> lk(super::mtx_);
    if (super::closed(lk)) return queue_op_status::closed;
    push(lk, std::forward<T>(elem));
    return queue_op_status::success;
  }
#endif
#endif

} //end concurrent namespace

using concurrent::sync_priority_queue;

} //end boost namespace
#include <boost/config/abi_suffix.hpp>

#endif
