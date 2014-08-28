#ifndef BOOST_THREAD_QUEUE_ADAPTOR_HPP
#define BOOST_THREAD_QUEUE_ADAPTOR_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/concurrent_queues/queue_base.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{

  template <typename Queue>
  class queue_adaptor : public queue_base<typename Queue::value_type>
  {
    Queue queue;
  public:
    typedef typename Queue::value_type value_type;
    typedef std::size_t size_type;

    // Constructors/Assignment/Destructors

    queue_adaptor()  {}

    // Observers
    bool empty() const  { return queue.empty(); }
    bool full() const { return queue.full(); }
    size_type size() const { return queue.size(); }
    bool closed() const { return queue.closed(); }

    // Modifiers
    void close() { queue.close(); }

    void push_back(const value_type& x) { queue.push_back(x); }

    void pull_front(value_type& x) { queue.pull_front(x); };
    // enable_if is_nothrow_copy_movable<value_type>
    value_type pull_front() { return queue.pull_front(); }

    queue_op_status try_push_back(const value_type& x) { return queue.try_push_back(x); }
    queue_op_status try_pull_front(value_type& x)  { return queue.try_pull_front(x); }

    queue_op_status nonblocking_push_back(const value_type& x) { return queue.nonblocking_push_back(x); }
    queue_op_status nonblocking_pull_front(value_type& x)  { return queue.nonblocking_pull_front(x); }

    queue_op_status wait_push_back(const value_type& x) { return queue.wait_push_back(x); }
    queue_op_status wait_pull_front(value_type& x) { return queue.wait_pull_front(x); }

#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    void push_back(BOOST_THREAD_RV_REF(value_type) x) { queue.push_back(boost::move(x)); }
    queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.try_push_back(boost::move(x)); }
    queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.nonblocking_push_back(boost::move(x)); }
    queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.wait_push_back(boost::move(x)); }
#endif
  };

}
using concurrent::queue_adaptor;

}

#include <boost/config/abi_suffix.hpp>

#endif
