// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_OS_MJM012402_HPP
#define BOOST_OS_MJM012402_HPP


namespace boost {

namespace threads {

namespace mac {

namespace os {


// functions to determine the OS environment.  With namespaces, you get a cute call:
//    mac::os::x

bool x();
long version();


} // namespace os

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_OS_MJM012402_HPP
