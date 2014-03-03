#ifndef BOOST_WIN32_GET_TICK_COUNT_HPP
#define BOOST_WIN32_GET_TICK_COUNT_HPP

// (C) Copyright 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail
  {
    namespace win32
    {
      typedef unsigned long long ticks_type;
      typedef ticks_type (__stdcall *gettickcount64fn)();
      typedef unsigned long (__stdcall *gettickcount32fn)();
      ticks_type BOOST_THREAD_DECL GetTickCount64();
    }
  }
}

#include <boost/config/abi_suffix.hpp>

#endif
