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

#if ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE) && \
    ! defined(BOOST_NO_CXX11_DECLTYPE_N3276) && \
    ! defined(BOOST_NO_CXX11_AUTO)

    //! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

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
#else
#if ! defined(BOOST_NO_SFINAE_EXPR) && \
    ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined BOOST_NO_CXX11_HDR_FUNCTIONAL

    //! defined(BOOST_NO_CXX11_RVALUE_REFERENCES)

    template <class Ret, class Fp, class ...Args>
    inline
    Ret invoke(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(Args) ...args)
    {
      return std::bind(boost::forward<Fp>(f), boost::forward<Args>(args)...)();
    }

#define BOOST_THREAD_PROVIDES_INVOKE_RET

#endif

#endif
      }
    }

#endif // header
