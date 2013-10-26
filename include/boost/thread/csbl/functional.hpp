// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_FUNCTIONAL_HPP
#define BOOST_CSBL_FUNCTIONAL_HPP

#include <boost/config.hpp>

#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
#include <boost/function.hpp>
#else
#include <functional>
#endif

namespace boost
{
  namespace csbl
  {
#ifdef BOOST_NO_CXX11_HDR_FUNCTIONAL
    using ::boost::function;

#else
    using ::std::function;

#endif

  }
}
#endif // header
