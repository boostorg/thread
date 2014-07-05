#ifndef BOOST_THREAD_PERMIT_HPP
#define BOOST_THREAD_PERMIT_HPP

//  permit.hpp
//
//  (C) Copyright 2014 Niall Douglas
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/platform.hpp>
#if defined(BOOST_THREAD_PLATFORM_WIN32)
#include <boost/thread/win32/permit.hpp>
#elif defined(BOOST_THREAD_PLATFORM_PTHREAD)
#include <boost/thread/pthread/permit.hpp>
#else
#error "Boost threads unavailable on this platform"
#endif

#endif
