// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2006 Roland Schwarz
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_ONCE_RS06092301_HPP
#define BOOST_ONCE_RS06092301_HPP

#include <boost/thread/pthread/config.hpp>

#include <pthread.h>

namespace boost {

typedef pthread_once_t once_flag;
#define BOOST_ONCE_INIT PTHREAD_ONCE_INIT

void BOOST_THREAD_DECL call_once(void (*func)(), once_flag& flag);

} // namespace boost

#endif // BOOST_ONCE_RS06092301_HPP
