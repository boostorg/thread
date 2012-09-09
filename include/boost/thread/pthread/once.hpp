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

#include <pthread.h>
#include <boost/assert.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/detail/delete.hpp>
// Force SIG_ATOMIC_MAX tobe defined
#define __STDC_LIMIT_MACROS
#include <csignal>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

#define BOOST_ONCE_INITIAL_FLAG_VALUE 0

  namespace thread_detail
  {
#ifdef SIG_ATOMIC_MAX
    typedef sig_atomic_t  uintmax_atomic_t;
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_MAX_C SIG_ATOMIC_MAX
#else
    typedef unsigned long  uintmax_atomic_t;
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(value) value##ul
    #define BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_MAX_C BOOST_THREAD_DETAIL_UINTMAX_ATOMIC_C2(~0)
#endif
  }

#ifdef BOOST_THREAD_PROVIDES_ONCE_CXX11

  struct once_flag
  {
      BOOST_THREAD_NO_COPYABLE(once_flag)
      BOOST_CONSTEXPR once_flag() BOOST_NOEXCEPT
        : epoch(BOOST_ONCE_INITIAL_FLAG_VALUE)
      {}
  private:
      volatile thread_detail::uintmax_atomic_t epoch;
      template<typename Function>
      friend
      void call_once(once_flag& flag,Function f);
  };

#else // BOOST_THREAD_PROVIDES_ONCE_CXX11

    struct once_flag
    {
      volatile thread_detail::uintmax_atomic_t epoch;
    };

#define BOOST_ONCE_INIT {BOOST_ONCE_INITIAL_FLAG_VALUE}
#endif // BOOST_THREAD_PROVIDES_ONCE_CXX11

    namespace detail
    {
        BOOST_THREAD_DECL thread_detail::uintmax_atomic_t& get_once_per_thread_epoch();
        BOOST_THREAD_DECL extern thread_detail::uintmax_atomic_t once_global_epoch;
        BOOST_THREAD_DECL extern pthread_mutex_t once_epoch_mutex;
        BOOST_THREAD_DECL extern pthread_cond_t once_epoch_cv;
    }

    // Based on Mike Burrows fast_pthread_once algorithm as described in
    // http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2444.html
    template<typename Function>
    void call_once(once_flag& flag,Function f)
    {
        static thread_detail::uintmax_atomic_t const uninitialized_flag=BOOST_ONCE_INITIAL_FLAG_VALUE;
        static thread_detail::uintmax_atomic_t const being_initialized=uninitialized_flag+1;
        thread_detail::uintmax_atomic_t const epoch=flag.epoch;
        thread_detail::uintmax_atomic_t& this_thread_epoch=detail::get_once_per_thread_epoch();

        if(epoch<this_thread_epoch)
        {
            pthread::pthread_mutex_scoped_lock lk(&detail::once_epoch_mutex);

            while(flag.epoch<=being_initialized)
            {
                if(flag.epoch==uninitialized_flag)
                {
                    flag.epoch=being_initialized;
#ifndef BOOST_NO_EXCEPTIONS
                    try // BOOST_NO_EXCEPTIONS protected
                    {
#endif
                        pthread::pthread_mutex_scoped_unlock relocker(&detail::once_epoch_mutex);
                        f();
#ifndef BOOST_NO_EXCEPTIONS
                    }
                    catch(...)  // BOOST_NO_EXCEPTIONS protected
                    {
                        flag.epoch=uninitialized_flag;
                        BOOST_VERIFY(!pthread_cond_broadcast(&detail::once_epoch_cv));
                        throw;  // BOOST_NO_EXCEPTIONS protected
                    }
#endif
                    flag.epoch=--detail::once_global_epoch;
                    BOOST_VERIFY(!pthread_cond_broadcast(&detail::once_epoch_cv));
                }
                else
                {
                    while(flag.epoch==being_initialized)
                    {
                        BOOST_VERIFY(!pthread_cond_wait(&detail::once_epoch_cv,&detail::once_epoch_mutex));
                    }
                }
            }
            this_thread_epoch=detail::once_global_epoch;
        }
    }
}

#include <boost/config/abi_suffix.hpp>

#endif
