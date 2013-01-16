#ifndef BOOST_THREAD_PTHREAD_ONCE_HPP
#define BOOST_THREAD_PTHREAD_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2007-8 Anthony Williams
//  (C) Copyright 2011-2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/detail/no_exceptions_support.hpp>

#include <boost/assert.hpp>
#include <boost/config/abi_prefix.hpp>

#include <boost/cstdint.hpp>
#include <pthread.h>
#include <csignal>

namespace boost
{

  struct once_flag;

  #define BOOST_ONCE_INITIAL_FLAG_VALUE 0

  namespace thread_detail
  {
    typedef boost::uint32_t  uintmax_atomic_t;
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(value) value##u
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_MAX_C BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(~0)

    inline bool enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
    inline void commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
    inline void rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;
  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    template<typename Function, class ...ArgTypes>
    inline void call_once(once_flag& flag, Function f, BOOST_THREAD_RV_REF(ArgTypes)... args);
#else
    template<typename Function>
    inline void call_once(once_flag& flag, Function f);
    template<typename Function, typename T1>
    inline void call_once(once_flag& flag, Function f, T1 p1);
    template<typename Function, typename T1, typename T2>
    inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2);
    template<typename Function, typename T1, typename T2, typename T3>
    inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3);
#endif

  struct once_flag
  {
      BOOST_THREAD_NO_COPYABLE(once_flag)
      BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT
        : epoch(BOOST_ONCE_INITIAL_FLAG_VALUE)
      {}
  private:
      volatile thread_detail::uintmax_atomic_t epoch;

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
      template<typename Function, class ...ArgTypes>
      friend void call_once(once_flag& flag, Function f, BOOST_THREAD_RV_REF(ArgTypes)... args);
#else
      template<typename Function>
      friend void call_once(once_flag& flag, Function f);
      template<typename Function, typename T1>
      friend void call_once(once_flag& flag, Function f, T1 p1);
      template<typename Function, typename T1, typename T2>
      friend void call_once(once_flag& flag, Function f, T1 p1, T2 p2);
      template<typename Function, typename T1, typename T2, typename T3>
      friend void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3);

      friend inline bool thread_detail::enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
      friend inline void thread_detail::commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
      friend inline void thread_detail::rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;

#endif

  };

#define BOOST_ONCE_INIT once_flag()

#else // BOOST_THREAD_PROVIDES_ONCE_CXX11

    struct once_flag
    {
      volatile thread_detail::uintmax_atomic_t epoch;
    };

#define BOOST_ONCE_INIT {BOOST_ONCE_INITIAL_FLAG_VALUE}
#endif // BOOST_THREAD_PROVIDES_ONCE_CXX11

    namespace thread_detail
    {
        BOOST_THREAD_DECL uintmax_atomic_t& get_once_per_thread_epoch();
        BOOST_THREAD_DECL extern uintmax_atomic_t once_global_epoch;
        BOOST_THREAD_DECL extern pthread_mutex_t once_epoch_mutex;
        BOOST_THREAD_DECL extern pthread_cond_t once_epoch_cv;
    }

    // Based on Mike Burrows fast_pthread_once algorithm as described in
    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2444.html

  namespace thread_detail
  {
    static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
    static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
    inline bool thread_detail::enter_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      thread_detail::uintmax_atomic_t const epoch=flag.epoch;
      thread_detail::uintmax_atomic_t& this_thread_epoch=thread_detail::get_once_per_thread_epoch();
      if(epoch<this_thread_epoch)
      {
          pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);

          while(flag.epoch<=being_initialized)
          {
              if(flag.epoch==uninitialized_flag)
              {
                  flag.epoch=being_initialized;
                  return false;
              }
          }
      }
      return false;
    }
    inline void thread_detail::commit_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);
        flag.epoch=uninitialized_flag;
      }
      BOOST_VERIFY(!pthread_cond_broadcast(&thread_detail::once_epoch_cv));
    }
    inline void thread_detail::rollback_once_region(once_flag& flag) BOOST_NOEXCEPT
    {
      {
        pthread::pthread_mutex_scoped_lock lk(&thread_detail::once_epoch_mutex);
        flag.epoch=--thread_detail::once_global_epoch;
      }
      BOOST_VERIFY(!pthread_cond_broadcast(&thread_detail::once_epoch_cv));
    }
  }
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

  template<typename Function, class ...ArgTypes>
  inline void call_once(once_flag& flag, Function f, BOOST_THREAD_RV_REF(ArgTypes)... args)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f(boost::forward<ArgTypes>(args)...);
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }
#else
  template<typename Function>
  inline void call_once(once_flag& flag, Function f)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f();
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1>
  inline void call_once(once_flag& flag, Function f, T1 p1)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f(p1);
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1, typename T2>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f(p1, p2);
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

  template<typename Function, typename T1, typename T2, typename T3>
  inline void call_once(once_flag& flag, Function f, T1 p1, T2 p2, T3 p3)
  {
    if (thread_detail::enter_once_region(flag))
    {
      BOOST_TRY
      {
        f(p1, p2, p3);
      }
      BOOST_CATCH (...)
      {
        thread_detail::rollback_once_region(flag);
        BOOST_RETHROW
      }
      BOOST_CATCH_END
      thread_detail::commit_once_region(flag);
    }
  }

#endif

}

#include <boost/config/abi_suffix.hpp>

#endif
