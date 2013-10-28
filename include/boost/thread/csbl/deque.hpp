// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_DEQUE_HPP
#define BOOST_CSBL_DEQUE_HPP

#include <boost/config.hpp>

#if defined BOOST_NO_CXX11_HDR_DEQUE || defined BOOST_NO_CXX11_RVALUE_REFERENCES
#include <boost/container/deque.hpp>
#else
#include <deque>
#endif

namespace boost
{
  namespace csbl
  {
#if defined BOOST_NO_CXX11_HDR_DEQUE || defined BOOST_NO_CXX11_RVALUE_REFERENCES
    using ::boost::container::deque;

#else
    using ::std::deque;

#endif

  }
}
#endif // header
