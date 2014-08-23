#ifndef BOOST_THREAD_QUEUE_BASE_HPP
#define BOOST_THREAD_QUEUE_BASE_HPP

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

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{

  template <typename ValueType>
  class queue_base
  {
  public:
    typedef ValueType value_type;
    typedef std::size_t size_type;

    // Constructors/Assignment/Destructors
    virtual ~queue_base() {};

    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void push_back(const value_type& x) = 0;
    virtual void push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;

    virtual void pull_front(value_type&) = 0;
    // enable_if is_nothrow_copy_movable<value_type>
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_push_back(const value_type& x) = 0;
    virtual queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_push_back(const value_type& x) = 0;
    virtual queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_push_back(const value_type& x) = 0;
    virtual queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status wait_pull_front(ValueType& elem) = 0;

  };


}
using concurrent::queue_base;

}

#include <boost/config/abi_suffix.hpp>

#endif
