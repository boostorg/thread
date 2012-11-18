//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright 2011-2012 Vicente J. Botet Escriba
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_DETAIL_IS_CONVERTIBLE_HPP
#define BOOST_THREAD_DETAIL_IS_CONVERTIBLE_HPP

#include <boost/type_traits/is_convertible.hpp>

namespace boost
{
  namespace thread_detail
  {
    template <typename T1, typename T2>
    struct is_convertible : boost::is_convertible<T1,T2> {};

    template <typename T1, typename T2>
    struct is_convertible<T1&, T2&> : boost::is_convertible<T1, T2> {};

  }

} // namespace boost


#endif //  BOOST_THREAD_DETAIL_MEMORY_HPP
