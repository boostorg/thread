// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

// This file is used to configure Boost.Threads during development
// in order to decouple dependency on any Boost release.  Once
// accepted into Boost these contents will be moved to <boost/config>
// or some other appropriate build configuration and all
// #include <boost/thread/config.hpp> statements will be changed
// accordingly.

#ifndef BOOST_THREAD_EXCEPTIONS_PDM070801_H
#define BOOST_THREAD_EXCEPTIONS_PDM070801_H

//  Sorry, but this class is used all over the place & I end up
//  with recursive headers if I don't separate it
#include <stdexcept>

namespace boost {

class lock_error : public std::runtime_error
{
public:
    lock_error();
};

} // namespace boost

#endif // BOOST_THREAD_CONFIG_PDM070801_H
