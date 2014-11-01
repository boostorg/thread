// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_SYNC_PRIORITY_QUEUE
#define BOOST_THREAD_SYNC_PRIORITY_QUEUE

#include <boost/thread/detail/config.hpp>

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
  {
  public:
    typedef chrono::steady_clock clock;
  protected:
    inline void wait_until_not_empty(unique_lock<mutex>& lk);
    inline bool wait_until_not_empty_or_closed(unique_lock<mutex>& lk);
    inline void throw_if_closed(unique_lock<mutex>&);
    bool closed(unique_lock<mutex> &) const
    {
      return _closed.load();
    }
    bool closed(lock_guard<mutex> &) const
    {
      return _closed.load();
    }
    inline bool empty(unique_lock<mutex>& ) const BOOST_NOEXCEPT
    {
      return _pq.empty();
    }
    inline bool empty(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return _pq.empty();
    }

  public:
    sync_priority_queue() : _closed(false) {}

    ~sync_priority_queue()
    {
      if(!_closed.load())
      {
        this->close();
      }
    }

    bool empty() const
    {
      lock_guard<mutex> lk(_qmutex);
      return empty(lk);
    }

    void close()
    {
      lock_guard<mutex> lk(_qmutex);
      _closed.store(true);
      _not_empty.notify_all();
    }

    bool closed() const
    {
      return _closed.load();
    }

    std::size_t size() const
    {
      return _pq.size();
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

  protected:
    atomic<bool> _closed;
    mutable mutex _qmutex;
    condition_variable _not_empty;
    std::priority_queue<ValueType,Container,Compare> _pq;

  private:
    sync_priority_queue(const sync_priority_queue&);
    sync_priority_queue& operator= (const sync_priority_queue&);
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    sync_priority_queue(sync_priority_queue&&);
    sync_priority_queue& operator= (sync_priority_queue&&);
#endif
  }; //end class

  template <class T,class Container, class Cmp>
  void sync_priority_queue<T,Container,Cmp>::throw_if_closed(unique_lock<mutex>& lk)
  {
    if (closed(lk))
    {
      BOOST_THROW_EXCEPTION( sync_queue_is_closed() );
    }
  }

  template <class T,class Container, class Cmp>
  void sync_priority_queue<T,Container,Cmp>::wait_until_not_empty(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (! empty(lk)) break;
      throw_if_closed(lk);
      //++waiting_empty_;
      _not_empty.wait(lk);
    }
  }

  template <class T,class Container, class Cmp>
  bool sync_priority_queue<T,Container,Cmp>::wait_until_not_empty_or_closed(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (! empty(lk)) break;
      if (closed(lk)) {return true;}
      //++waiting_empty_;
      _not_empty.wait(lk);
    }
    return false;
  }

  template <class T,class Container, class Cmp>
  T sync_priority_queue<T,Container,Cmp>::pull()
  {
    unique_lock<mutex> lk(_qmutex);
    wait_until_not_empty(lk);
    T first = _pq.top();
    _pq.pop();
    return first;
  }

  template <class T, class Cont,class Cmp>
  optional<T>
  sync_priority_queue<T,Cont,Cmp>::pull_until(const clock::time_point& tp)
  {
    unique_lock<mutex> lk(_qmutex);
    while(_pq.empty())
    {
      if(_closed.load()) throw std::exception();
      if(_not_empty.wait_until(lk, tp) == cv_status::timeout )
      {
        return optional<T>();
      }
    }
    optional<T> fst( _pq.top() );
    _pq.pop();
    return fst;
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
    lock_guard<mutex> lk(_qmutex);
    if(_pq.empty())
    {
      return optional<T>();
    }
    else
    {
      optional<T> fst( _pq.top() );
      _pq.pop();
      return fst;
    }
  }

  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(const T& elem)
  {
    lock_guard<mutex> lk(_qmutex);
    _pq.push(elem);
    _not_empty.notify_one();
  }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
  template <class T, class Container,class Cmp>
  void sync_priority_queue<T,Container,Cmp>::push(T&& elem)
  {
    lock_guard<mutex> lk(_qmutex);
    _pq.emplace(elem);
    _not_empty.notify_one();
  }
#endif

  template <class T, class Container,class Cmp>
  optional<T>
  sync_priority_queue<T,Container,Cmp>::try_pull()
  {
    unique_lock<mutex> lk(_qmutex, try_to_lock);
    if(lk.owns_lock())
    {
      wait_until_not_empty(lk);
      optional<T> fst( _pq.top() );
      _pq.pop();
      return fst;
    }
    return optional<T>();
  }

  template <class T, class Container,class Cmp>
  optional<T>
  sync_priority_queue<T,Container,Cmp>::try_pull_no_wait()
  {
    unique_lock<mutex> lk(_qmutex, try_to_lock);
    if(lk.owns_lock() && !_pq.empty())
    {
      optional<T> fst( _pq.top() );
      _pq.pop();
      return fst;
    }
    return optional<T>();
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <class T, class Container,class Cmp>
  bool sync_priority_queue<T,Container,Cmp>::try_push(const T& elem)
  {
    unique_lock<mutex> lk(_qmutex, try_to_lock);
    if(lk.owns_lock())
    {
      _pq.push(elem);
      _not_empty.notify_one();
      return true;
    }
    return false;
  }
#else
  template <class T, class Container,class Cmp>
  queue_op_status sync_priority_queue<T,Container,Cmp>::try_push(const T& elem)
  {
    lock_guard<mutex> lk(_qmutex);
    if (closed(lk)) return queue_op_status::closed;
    else {
      _pq.push(elem);
    }
    _not_empty.notify_one();
    return queue_op_status::success;
  }
#endif

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <class T, class Container,class Cmp>
  bool sync_priority_queue<T,Container,Cmp>::try_push(T&& elem)
  {
    unique_lock<mutex> lk(_qmutex, try_to_lock);
    if(lk.owns_lock())
    {
      _pq.emplace(elem);
      _not_empty.notify_one();
      return true;
    }
    return false;
  }
#else
  template <class T, class Container,class Cmp>
  queue_op_status sync_priority_queue<T,Container,Cmp>::try_push(T&& elem)
  {
    lock_guard<mutex> lk(_qmutex);
    if (closed(lk)) return queue_op_status::closed;
    else
    {
      _pq.emplace(elem);
    }
    _not_empty.notify_one();
    return queue_op_status::success;
  }
#endif
#endif

} //end concurrent namespace

using concurrent::sync_priority_queue;

} //end boost namespace
#include <boost/config/abi_suffix.hpp>

#endif
