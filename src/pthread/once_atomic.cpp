//  (C) Copyright 2013 Andrey Semashev
//  (C) Copyright 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define __STDC_CONSTANT_MACROS
#include <boost/thread/detail/config.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/atomic.hpp>
#include <boost/memory_order.hpp>
#include <pthread.h>

namespace boost
{
  namespace thread_detail
  {

    enum flag_states
    {
      uninitialized, in_progress, initialized
    };

#if BOOST_ATOMIC_INT_LOCK_FREE == 2
    typedef unsigned int atomic_int_type;
#elif BOOST_ATOMIC_SHORT_LOCK_FREE == 2
    typedef unsigned short atomic_int_type;
#elif BOOST_ATOMIC_CHAR_LOCK_FREE == 2
    typedef unsigned char atomic_int_type;
#elif BOOST_ATOMIC_LONG_LOCK_FREE == 2
    typedef unsigned long atomic_int_type;
#elif defined(BOOST_HAS_LONG_LONG) && BOOST_ATOMIC_LLONG_LOCK_FREE == 2
    typedef ulong_long_type atomic_int_type;
#else
    // All tested integer types are not atomic, the spinlock pool will be used
    typedef unsigned int atomic_int_type;
#endif

    typedef boost::atomic<atomic_int_type> atomic_type
    //#if defined(__GNUC__)
    //    __attribute__((may_alias))
    //#endif
    ;
    BOOST_STATIC_ASSERT_MSG(sizeof(once_flag) >= sizeof(atomic_type), "Boost.Thread: unsupported platform");

    static pthread_mutex_t once_mutex = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t once_cv = PTHREAD_COND_INITIALIZER;

    BOOST_THREAD_DECL bool enter_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      atomic_type& f = reinterpret_cast< atomic_type& >(flag.storage);
      if (f.load(memory_order_acquire) != initialized)
      {
        pthread::pthread_mutex_scoped_lock lk(&once_mutex);
        if (f.load(memory_order_acquire) != initialized)
        {
          while (true)
          {
            atomic_int_type expected = uninitialized;
            if (f.compare_exchange_strong(expected, in_progress, memory_order_acq_rel, memory_order_acquire))
            {
              // We have set the flag to in_progress
              return true;
            }
            else if (expected == initialized)
            {
              // Another thread managed to complete the initialization
              return false;
            }
            else
            {
              // Wait until the initialization is complete
              //pthread::pthread_mutex_scoped_lock lk(&once_mutex);
              BOOST_VERIFY(!pthread_cond_wait(&once_cv, &once_mutex));
            }
          }
        }
      }
      return false;
    }

    BOOST_THREAD_DECL void commit_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      atomic_type& f = reinterpret_cast< atomic_type& >(flag.storage);
      {
        pthread::pthread_mutex_scoped_lock lk(&once_mutex);
        f.store(initialized, memory_order_release);
      }
      BOOST_VERIFY(!pthread_cond_broadcast(&once_cv));
    }

    BOOST_THREAD_DECL void rollback_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      atomic_type& f = reinterpret_cast< atomic_type& >(flag.storage);
      {
        pthread::pthread_mutex_scoped_lock lk(&once_mutex);
        f.store(uninitialized, memory_order_release);
      }
      BOOST_VERIFY(!pthread_cond_broadcast(&once_cv));
    }

  } // namespace thread_detail

} // namespace boost
