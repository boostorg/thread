// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "scoped_critical_region.hpp"

#include "init.hpp"


#include <cassert>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


scoped_critical_region::scoped_critical_region():
    m_pCriticalRegionID(kInvalidID)
{
    static bool bIgnored = thread_init();
    OSStatus lStatus = MPCreateCriticalRegion(&m_pCriticalRegionID);
    if(lStatus != noErr || m_pCriticalRegionID == kInvalidID)
        throw(thread_resource_error());
}

scoped_critical_region::~scoped_critical_region()
{
    OSStatus lStatus = MPDeleteCriticalRegion(m_pCriticalRegionID);
    assert(lStatus == noErr);
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
