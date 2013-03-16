// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
// The invoke code is based on the one from libcxx.
//===----------------------------------------------------------------------===//

#ifndef BOOST_THREAD_DETAIL_INVOKE_HPP
#define BOOST_THREAD_DETAIL_INVOKE_HPP

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/thread/detail/move.hpp>
#ifndef BOOST_NO_CXX11_HDR_FUNCTIONAL
#include <functional>
#endif

namespace boost
{
  namespace detail
  {

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#if ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE_N3276) && \
    ! defined(BOOST_NO_CXX11_AUTO)

#define BOOST_THREAD_PROVIDES_INVOKE

    //      // bullets 1 and 2

    template <class Fp, class A0, class ...Args>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(Args) ...args)
        -> decltype((boost::forward<A0>(a0).*f)(boost::forward<Args>(args)...))
    {
        return (boost::forward<A0>(a0).*f)(boost::forward<Args>(args)...);
    }

    template <class Fp, class A0, class ...Args>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(Args) ...args)
        -> decltype(((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(args)...))
    {
        return ((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(args)...);
    }

    // bullets 3 and 4

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype(boost::forward<A0>(a0).*f)
    {
        return boost::forward<A0>(a0).*f;
    }

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype((*boost::forward<A0>(a0)).*f)
    {
        return (*boost::forward<A0>(a0)).*f;
    }

    // bullet 5

    template <class Fp, class ...Args>
    inline
    auto invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(Args) ...args)
    -> decltype(boost::forward<Fp>(f)(boost::forward<Args>(args)...))
    {
      return boost::forward<Fp>(f)(boost::forward<Args>(args)...);
    }
#elif ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined BOOST_NO_CXX11_HDR_FUNCTIONAL

    template <class Ret, class Fp, class ...Args>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(Args) ...args)
    {
      return std::bind(boost::forward<Fp>(f), boost::forward<Args>(args)...)();
    }

#define BOOST_THREAD_PROVIDES_INVOKE_RET

#endif

#else //! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#if ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE_N3276) && \
    ! defined(BOOST_NO_CXX11_AUTO)

#define BOOST_THREAD_PROVIDES_INVOKE

    //      // bullets 1 and 2

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype((boost::forward<A0>(a0).*f)())
    {
        return (boost::forward<A0>(a0).*f)();
    }
    template <class Fp, class A0, class A1>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(A1) a1)
        -> decltype((boost::forward<A0>(a0).*f)(boost::forward<Args>(a1)))
    {
        return (boost::forward<A0>(a0).*f)(boost::forward<Args>(a1));
    }
    template <class Fp, class A0, class A1, class A2>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2)
        -> decltype((boost::forward<A0>(a0).*f)(boost::forward<Args>(a1), boost::forward<Args>(a2)))
    {
        return (boost::forward<A0>(a0).*f)(boost::forward<Args>(a1), boost::forward<Args>(a2));
    }

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype(((*boost::forward<A0>(a0)).*f)())
    {
        return ((*boost::forward<A0>(a0)).*f)();
    }
    template <class Fp, class A0, class A1>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(A1) a1)
        -> decltype(((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(a1)))
    {
        return ((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(a1));
    }
    template <class Fp, class A0, class A1>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2)
        -> decltype(((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(a1), boost::forward<Args>(a2)))
    {
        return ((*boost::forward<A0>(a0)).*f)(boost::forward<Args>(a1), boost::forward<Args>(a2));
    }

    // bullets 3 and 4

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype(boost::forward<A0>(a0).*f)
    {
        return boost::forward<A0>(a0).*f;
    }

    template <class Fp, class A0>
    inline
    auto
    invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A0) a0)
        -> decltype((*boost::forward<A0>(a0)).*f)
    {
        return (*boost::forward<A0>(a0)).*f;
    }

    // bullet 5

    template <class Fp>
    inline
    auto invoke(BOOST_THREAD_RV_REF(Fp) f)
    -> decltype(boost::forward<Fp>(f)())
    {
      return boost::forward<Fp>(f)();
    }
    template <class Fp, class A1>
    inline
    auto invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(Args) a1)
    -> decltype(boost::forward<Fp>(f)(boost::forward<A1>(a1)))
    {
      return boost::forward<Fp>(f)(boost::forward<A1>(a1));
    }    template <class Fp, class A1, class A2>
    inline
    auto invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2)
    -> decltype(boost::forward<Fp>(f)(boost::forward<A1>(a1), boost::forward<A2>(a2)))
    {
      return boost::forward<Fp>(f)(boost::forward<A1>(a1), boost::forward<A2>(a2));
    }
    template <class Fp, class A1, class A2, class A3>
    inline
    auto invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2, BOOST_THREAD_RV_REF(A3) a3)
    -> decltype(boost::forward<Fp>(f)(boost::forward<A1>(a1), boost::forward<A2>(a2), boost::forward<A3>(a3)))
    {
      return boost::forward<Fp>(f)(boost::forward<A1>(a1), boost::forward<A2>(a2), boost::forward<A3>(a3));
    }


#elif ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined BOOST_NO_CXX11_HDR_FUNCTIONAL

    template <class Ret, class Fp>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f)
    {
      return f();
    }
    template <class Ret, class Fp, class A1>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A1) a1)
    {
      return std::bind(boost::forward<Fp>(f), boost::forward<A1>(a1))();
    }
    template <class Ret, class Fp, class A1, class A2>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2)
    {
      return std::bind(boost::forward<Fp>(f), boost::forward<A1>(a1), boost::forward<A2>(a2))();
    }
    template <class Ret, class Fp, class A1, class A2, class A3>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(A1) a1, BOOST_THREAD_RV_REF(A2) a2, BOOST_THREAD_RV_REF(A3) a3)
    {
      return std::bind(boost::forward<Fp>(f), boost::forward<A1>(a1), boost::forward<A2>(a2), boost::forward<A3>(a3))();
    }

#define BOOST_THREAD_PROVIDES_INVOKE_RET

#endif


#endif // ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
      }
    }

#endif // header
