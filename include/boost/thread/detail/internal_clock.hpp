// Copyright (C) 2017
// Vicente J. Botet Escriba
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_DETAIL_INTERNAL_CLOCK_HPP
#define BOOST_THREAD_DETAIL_INTERNAL_CLOCK_HPP

#include <boost/thread/detail/config.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost {
namespace thread_detail {

  #ifdef BOOST_THREAD_USES_CHRONO
  #ifdef BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
          typedef chrono::steady_clock internal_clock_t;
  #else
          typedef chrono::system_clock internal_clock_t;
  #endif
  #endif

} // namespace thread_detail
} // namespace boost

#include <boost/config/abi_suffix.hpp>

#endif // header
