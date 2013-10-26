// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_TUPLE_HPP
#define BOOST_CSBL_TUPLE_HPP

#include <boost/config.hpp>

#ifdef BOOST_NO_CXX11_HDR_TUPLE
#include <boost/tuple/tuple.hpp>
#else
#include <tuple>
#endif

namespace boost
{
  namespace csbl
  {
#ifdef BOOST_NO_CXX11_HDR_TUPLE
    using ::boost::tuple;

#else
    using ::std::tuple;

#endif

  }
}
#endif // header
