// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_DETAIL_DELETE_HPP
#define BOOST_THREAD_DETAIL_DELETE_HPP

#include <boost/config.hpp>

#ifndef BOOST_NO_DELETED_FUNCTIONS
#define BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
      CLASS(CLASS const&) = delete; \

#define BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS) \
      CLASS& operator=(CLASS const&) = delete;

#else // BOOST_NO_DELETED_FUNCTIONS
#define BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
    private: \
      CLASS(CLASS&); \
    public:

#define BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS) \
    private: \
      CLASS& operator=(CLASS&); \
    public:
#endif // BOOST_NO_DELETED_FUNCTIONS

#define BOOST_THREAD_NO_COPYABLE(CLASS) \
    BOOST_THREAD_DELETE_COPY_CTOR(CLASS) \
    BOOST_THREAD_DELETE_COPY_ASSIGN(CLASS)

#endif // BOOST_THREAD_DETAIL_DELETE_HPP
