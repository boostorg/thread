#ifndef BOOST_THREAD_CONCURRENT_QUEUES_SYNC_QUEUE_HPP
#define BOOST_THREAD_CONCURRENT_QUEUES_SYNC_QUEUE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2013-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/concurrent_queues/detail/sync_queue_base.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/csbl/deque.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/throw_exception.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
  template <typename ValueType>
  class sync_queue
    : public detail::sync_queue_base<ValueType, csbl::deque<ValueType> >
  {
    typedef detail::sync_queue_base<ValueType, csbl::deque<ValueType> >  super;

  public:
    typedef ValueType value_type;
    //typedef typename super::value_type value_type; // fixme
    typedef typename super::underlying_queue_type underlying_queue_type;
    typedef typename super::size_type size_type;
    typedef typename super::op_status op_status;

    // Constructors/Assignment/Destructors
    BOOST_THREAD_NO_COPYABLE(sync_queue)
    inline sync_queue();
    //template <typename Range>
    //inline explicit sync_queue(Range range);
    inline ~sync_queue();

    // Modifiers
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline void push(const value_type& x);
    inline bool try_push(const value_type& x);
    inline bool try_push(no_block_tag, const value_type& x);
    inline void push(BOOST_THREAD_RV_REF(value_type) x);
    inline bool try_push(BOOST_THREAD_RV_REF(value_type) x);
    inline bool try_push(no_block_tag, BOOST_THREAD_RV_REF(value_type) x);
#endif
    inline void push_back(const value_type& x);
    inline queue_op_status try_push_back(const value_type& x);
    inline queue_op_status nonblocking_push_back(const value_type& x);
    inline queue_op_status wait_push_back(const value_type& x);
    inline void push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x);

    // Observers/Modifiers
#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline void pull(value_type&);
    inline void pull(ValueType& elem, bool & closed);
    // enable_if is_nothrow_copy_movable<value_type>
    inline value_type pull();
    inline shared_ptr<ValueType> ptr_pull();
#endif
    inline void pull_front(value_type&);
    // enable_if is_nothrow_copy_movable<value_type>
    inline value_type pull_front();

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline bool try_pull(value_type&);
    inline bool try_pull(no_block_tag,value_type&);
    inline shared_ptr<ValueType> try_pull();
#endif
    inline queue_op_status try_pull_front(value_type&);
    inline queue_op_status nonblocking_pull_front(value_type&);
    inline queue_op_status wait_pull_front(ValueType& elem);

  private:

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline bool try_pull(value_type& x, unique_lock<mutex>& lk);
    inline bool try_push(const value_type& x, unique_lock<mutex>& lk);
    inline bool try_push(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);
    inline shared_ptr<value_type> try_pull(unique_lock<mutex>& lk);
#endif
    inline queue_op_status try_pull_front(value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status wait_pull_front(value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status try_push_back(const value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status wait_push_back(const value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);
    inline queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline void pull(value_type& elem, unique_lock<mutex>& )
    {
      elem = boost::move(super::data_.front());
      super::data_.pop_front();
    }
    inline value_type pull(unique_lock<mutex>& )
    {
      value_type e = boost::move(super::data_.front());
      super::data_.pop_front();
      return boost::move(e);
    }
    inline boost::shared_ptr<value_type> ptr_pull(unique_lock<mutex>& )
    {
      shared_ptr<value_type> res = make_shared<value_type>(boost::move(super::data_.front()));
      super::data_.pop_front();
      return res;
    }
#endif
    inline void pull_front(value_type& elem, unique_lock<mutex>& )
    {
      elem = boost::move(super::data_.front());
      super::data_.pop_front();
    }
    inline value_type pull_front(unique_lock<mutex>& )
    {
      value_type e = boost::move(super::data_.front());
      super::data_.pop_front();
      return boost::move(e);
    }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
    inline void push(const value_type& elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(elem);
      super::notify_not_empty_if_needed(lk);
    }

    inline void push(BOOST_THREAD_RV_REF(value_type) elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(boost::move(elem));
      super::notify_not_empty_if_needed(lk);
    }
#endif
    inline void push_back(const value_type& elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(elem);
      super::notify_not_empty_if_needed(lk);
    }

    inline void push_back(BOOST_THREAD_RV_REF(value_type) elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(boost::move(elem));
      super::notify_not_empty_if_needed(lk);
    }
  };

  template <typename ValueType>
  sync_queue<ValueType>::sync_queue() :
    super()
  {
  }

//  template <typename ValueType>
//  template <typename Range>
//  explicit sync_queue<ValueType>::sync_queue(Range range) :
//    waiting_empty_(0), data_(), closed_(false)
//  {
//    try
//    {
//      typedef typename Range::iterator iterator_t;
//      iterator_t first = boost::begin(range);
//      iterator_t end = boost::end(range);
//      for (iterator_t cur = first; cur != end; ++cur)
//      {
//        data_.push(boost::move(*cur));;
//      }
//      notify_not_empty_if_needed(lk);
//    }
//    catch (...)
//    {
//      delete[] data_;
//    }
//  }

  template <typename ValueType>
  sync_queue<ValueType>::~sync_queue()
  {
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_pull(ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::empty(lk))
    {
      super::throw_if_closed(lk);
      return false;
    }
    pull(elem, lk);
    return true;
  }
  template <typename ValueType>
  shared_ptr<ValueType> sync_queue<ValueType>::try_pull(unique_lock<mutex>& lk)
  {
    if (super::empty(lk))
    {
      super::throw_if_closed(lk);
      return shared_ptr<ValueType>();
    }
    return ptr_pull(lk);
  }
#endif
  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_pull_front(ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::empty(lk))
    {
      if (super::closed(lk)) return queue_op_status::closed;
      return queue_op_status::empty;
    }
    pull_front(elem, lk);
    return queue_op_status::success;
  }
  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_pull_front(ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::empty(lk))
    {
      if (super::closed(lk)) return queue_op_status::closed;
    }
    bool has_been_closed = super::wait_until_not_empty_or_closed(lk);
    if (has_been_closed) return queue_op_status::closed;
    pull_front(elem, lk);
    return queue_op_status::success;
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_pull(ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      return try_pull(elem, lk);
  }
#endif
  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_pull_front(elem, lk);
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_pull_front(elem, lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_pull(no_block_tag,ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_, try_to_lock);
      if (!lk.owns_lock())
      {
        return false;
      }
      return try_pull(elem, lk);
  }
  template <typename ValueType>
  boost::shared_ptr<ValueType> sync_queue<ValueType>::try_pull()
  {
      unique_lock<mutex> lk(super::mtx_);
      return try_pull(lk);
  }
#endif
  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::nonblocking_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock())
    {
      return queue_op_status::busy;
    }
    return try_pull_front(elem, lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  void sync_queue<ValueType>::pull(ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::wait_until_not_empty(lk);
      pull(elem, lk);
  }
  template <typename ValueType>
  void sync_queue<ValueType>::pull(ValueType& elem, bool & has_been_closed)
  {
      unique_lock<mutex> lk(super::mtx_);
      has_been_closed = super::wait_until_not_empty_or_closed(lk);
      if (has_been_closed) {return;}
      pull(elem, lk);
  }

  // enable if ValueType is nothrow movable
  template <typename ValueType>
  ValueType sync_queue<ValueType>::pull()
  {
      unique_lock<mutex> lk(super::mtx_);
      super::wait_until_not_empty(lk);
      return pull(lk);
  }
  template <typename ValueType>
  boost::shared_ptr<ValueType> sync_queue<ValueType>::ptr_pull()
  {
      unique_lock<mutex> lk(super::mtx_);
      super::wait_until_not_empty(lk);
      return ptr_pull(lk);
  }
#endif

  template <typename ValueType>
  void sync_queue<ValueType>::pull_front(ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::wait_until_not_empty(lk);
      pull_front(elem, lk);
  }

  // enable if ValueType is nothrow movable
  template <typename ValueType>
  ValueType sync_queue<ValueType>::pull_front()
  {
      unique_lock<mutex> lk(super::mtx_);
      super::wait_until_not_empty(lk);
      return pull_front(lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(const ValueType& elem, unique_lock<mutex>& lk)
  {
    super::throw_if_closed(lk);
    push(elem, lk);
    return true;
  }

  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(const ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      return try_push(elem, lk);
  }
#endif

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_push_back(const ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(elem, lk);
    return queue_op_status::success;
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_push_back(elem, lk);
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_push_back(const ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(elem, lk);
    return queue_op_status::success;
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_push_back(elem, lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(no_block_tag, const ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_, try_to_lock);
      if (!lk.owns_lock()) return false;
      return try_push(elem, lk);
  }
#endif
  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::nonblocking_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock()) return queue_op_status::busy;
    return try_push_back(elem, lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  void sync_queue<ValueType>::push(const ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push(elem, lk);
  }
#endif

  template <typename ValueType>
  void sync_queue<ValueType>::push_back(const ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push_back(elem, lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    super::throw_if_closed(lk);
    push(boost::move(elem), lk);
    return true;
  }

  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(BOOST_THREAD_RV_REF(ValueType) elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      return try_push(boost::move(elem), lk);
  }
#endif

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_push_back(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(boost::move(elem), lk);
    return queue_op_status::success;
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::try_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_push_back(boost::move(elem), lk);
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_push_back(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(boost::move(elem), lk);
    return queue_op_status::success;
  }

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::wait_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_push_back(boost::move(elem), lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  bool sync_queue<ValueType>::try_push(no_block_tag, BOOST_THREAD_RV_REF(ValueType) elem)
  {
      unique_lock<mutex> lk(super::mtx_, try_to_lock);
      if (!lk.owns_lock())
      {
        return false;
      }
      return try_push(boost::move(elem), lk);
  }
#endif

  template <typename ValueType>
  queue_op_status sync_queue<ValueType>::nonblocking_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock())
    {
      return queue_op_status::busy;
    }
    return try_push_back(boost::move(elem), lk);
  }

#ifndef BOOST_THREAD_QUEUE_DEPRECATE_OLD
  template <typename ValueType>
  void sync_queue<ValueType>::push(BOOST_THREAD_RV_REF(ValueType) elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push(boost::move(elem), lk);
  }
#endif

  template <typename ValueType>
  void sync_queue<ValueType>::push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push_back(boost::move(elem), lk);
  }

  template <typename ValueType>
  sync_queue<ValueType>& operator<<(sync_queue<ValueType>& sbq, BOOST_THREAD_RV_REF(ValueType) elem)
  {
    sbq.push_back(boost::move(elem));
    return sbq;
  }

  template <typename ValueType>
  sync_queue<ValueType>& operator<<(sync_queue<ValueType>& sbq, ValueType const&elem)
  {
    sbq.push_back(elem);
    return sbq;
  }

  template <typename ValueType>
  sync_queue<ValueType>& operator>>(sync_queue<ValueType>& sbq, ValueType &elem)
  {
    sbq.pull_front(elem);
    return sbq;
  }

}
using concurrent::sync_queue;

}

#include <boost/config/abi_suffix.hpp>

#endif
