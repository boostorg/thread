// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "os.hpp"


#include <cassert>

#include <Gestalt.h>


namespace boost {

namespace threads {

namespace mac {

namespace os {


// read the OS version from Gestalt
static inline long get_version()
{
    long lVersion;
    OSErr nErr = Gestalt(gestaltSystemVersion, &lVersion);
    assert(nErr == noErr);
    return(lVersion);
}


// check if we're running under Mac OS X and cache that information
bool x()
{
    static bool bX = (version() >= 0x1000);
    return(bX);
}


// read the OS version and cache it
long version()
{
    static long lVersion = get_version();
    return(lVersion);
}


} // namespace os

} // namespace mac

} // namespace threads

} // namespace boost
