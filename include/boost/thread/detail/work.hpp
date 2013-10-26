//  (C) Copyright 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_DETAIL_WORK_HPP
#define BOOST_THREAD_DETAIL_WORK_HPP

#define BOOST_THREAD_USES_NULLARY_FUNCTION_AS_WORK

#ifdef BOOST_THREAD_USES_NULLARY_FUNCTION_AS_WORK
#include <boost/thread/detail/nullary_function.hpp>
#else
#include <boost/thread/detail/function_wrapper.hpp>
#endif

namespace boost
{
  namespace thread_detail
  {

#ifdef BOOST_THREAD_USES_NULLARY_FUNCTION_AS_WORK
    typedef detail::nullary_function<void()> work;
#else
    typedef detail::function_wrapper work;
#endif
  }

} // namespace boost

#endif //  BOOST_THREAD_DETAIL_MEMORY_HPP
