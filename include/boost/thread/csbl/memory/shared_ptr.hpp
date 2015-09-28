// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2014/10 Vicente J. Botet Escriba
//   Creation.

#ifndef BOOST_CSBL_MEMORY_SHARED_PTR_HPP
#define BOOST_CSBL_MEMORY_SHARED_PTR_HPP

#include <boost/thread/csbl/memory/config.hpp>

//#if defined BOOST_NO_CXX11_SMART_PTR
#if 1

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace boost
{
  template<class T> bool atomic_compare_exchange_strong( shared_ptr<T> * p, shared_ptr<T> * v, shared_ptr<T> w )
  {
    return atomic_compare_exchange(p,v,w);
  }

  namespace csbl
  {
    using ::boost::shared_ptr;
    using ::boost::make_shared;
    using ::boost::dynamic_pointer_cast;
    using ::boost::enable_shared_from_this;
    using ::boost::atomic_load;
    using ::boost::atomic_compare_exchange_strong;
  }
}

#else

#include <boost/shared_ptr.hpp>

namespace boost
{
  namespace csbl
  {
    using std::shared_ptr;
    using std::make_shared;
    using std::dynamic_pointer_cast;
    using std::enable_shared_from_this;
    using std::atomic_load;
    using std::atomic_compare_exchange_strong;
  }
}

#endif
#endif // header
