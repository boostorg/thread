#ifndef BOOST_THREAD_PTHREAD_ONCE_ATOMIC_HPP
#define BOOST_THREAD_PTHREAD_ONCE_ATOMIC_HPP

//  once.hpp
//
//  (C) Copyright 2013 Andrey Semashev
//  (C) Copyright 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#include <boost/cstdint.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/detail/no_exceptions_support.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  struct once_flag;

  namespace thread_detail
  {
    BOOST_THREAD_DECL bool enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
    BOOST_THREAD_DECL void commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
    BOOST_THREAD_DECL void rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;
  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11

  struct once_flag
  {
    BOOST_THREAD_NO_COPYABLE(once_flag)
    BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT : storage(0)
    {
    }

  private:
  #if defined(__GNUC__)
    __attribute__((may_alias))
  #endif
    uintmax_t storage;

    friend BOOST_THREAD_DECL bool thread_detail::enter_once_region(once_flag& flag) BOOST_NOEXCEPT;
    friend BOOST_THREAD_DECL void thread_detail::commit_once_region(once_flag& flag) BOOST_NOEXCEPT;
    friend BOOST_THREAD_DECL void thread_detail::rollback_once_region(once_flag& flag) BOOST_NOEXCEPT;
  };

#define BOOST_ONCE_INIT boost::once_flag()

#else // BOOST_THREAD_PROVIDES_ONCE_CXX11
  struct once_flag
  {
  #if defined(__GNUC__)
    __attribute__((may_alias))
  #endif
    uintmax_t storage;
  };

  #define BOOST_ONCE_INIT {0}

#endif // BOOST_THREAD_PROVIDES_ONCE_CXX11

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

  template<typename Function, class ...ArgTypes>
  inline void call_once(once_flag& flag, BOOST_THREAD_RV_REF(Function) f, BOOST_THREAD_RV_REF(ArgTypes)... args)
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

