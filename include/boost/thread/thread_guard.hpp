// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2009-2012 Anthony Williams
// (C) Copyright 2012 Vicente J. Botet Escriba

// Based on the Anthony's idea of thread_joiner in CCiA

#ifndef BOOST_THREAD_THREAD_GUARD_HPP
#define BOOST_THREAD_THREAD_GUARD_HPP

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  /**
   * Non-copyable RAII scoped thread guard joiner which join the thread if joinable when destroyed.
   */
  class strict_thread_joiner
  {
    thread& t;
  public:
    BOOST_THREAD_MOVABLE_ONLY( strict_thread_joiner)

    explicit strict_thread_joiner(thread& t_) :
    t(t_)
    {
    }
    ~strict_thread_joiner()
    {
      if (t.joinable())
      {
        t.join();
      }
    }
  };

  /**
   * MoveOnly RAII scoped thread guard joiner which join the thread if joinable when destroyed.
   */
  class thread_joiner
  {
    thread* t;
  public:
    BOOST_THREAD_MOVABLE_ONLY( thread_joiner)

    explicit thread_joiner(thread& t_) :
      t(&t_)
    {
    }

    thread_joiner(BOOST_RV_REF(thread_joiner) x) :
    t(x.t)
    {
      x.t = 0;
    }

    thread_joiner& operator=(BOOST_RV_REF(thread_joiner) x)
    {
      t = x.t;
      x.t = 0;
      return *this;
    }

    void swap(thread_joiner& rhs)BOOST_NOEXCEPT
    {
      thread* tmp=t;
      t = rhs.t;
      rhs.t = tmp;
    }

    ~thread_joiner()
    {
      if (t)
      {
        if (t->joinable())
        {
          t->join();
        }
      }
    }
  };

}
#include <boost/config/abi_suffix.hpp>

#endif
