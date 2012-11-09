// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_LOCK_GUARD_HPP
#define BOOST_THREAD_LOCK_GUARD_HPP

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/lock_options.hpp>
#if ! defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
#include <boost/thread/is_locked_by_this_thread.hpp>
#endif
#include <boost/assert.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  template <typename Mutex>
  class lock_guard
  {
  private:
    Mutex& m;

  public:
    typedef Mutex mutex_type;
    BOOST_THREAD_NO_COPYABLE( lock_guard)

    explicit lock_guard(Mutex& m_) :
      m(m_)
    {
      m.lock();
    }
    lock_guard(Mutex& m_, adopt_lock_t) :
      m(m_)
    {
#if ! defined BOOST_THREAD_PROVIDES_NESTED_LOCKS
      BOOST_ASSERT(is_locked_by_this_thread(m));
#endif
    }
    ~lock_guard()
    {
      m.unlock();
    }
  };

}
#include <boost/config/abi_suffix.hpp>

#endif
