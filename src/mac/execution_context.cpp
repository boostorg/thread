// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include <Debugging.h>
#include <Multiprocessing.h>

#include "execution_context.hpp"
#include "init.hpp"


namespace boost {

namespace threads {

namespace mac {


execution_context_t execution_context()
{
// make sure that MP services are available the first time through
    static bool bIgnored = detail::thread_init();

// first check if we're an MP task
    if(MPTaskIsPreemptive(kInvalidID))
    {
        return(k_eExecutionContextMPTask);
    }

#if TARGET_CARBON
// Carbon has TaskLevel
    UInt32 ulLevel = TaskLevel();

    if(ulLevel == 0UL)
    {
        return(k_eExecutionContextSystemTask);
    }

    if(ulLevel & kInDeferredTaskMask)
    {
        return(k_eExecutionContextDeferredTask);
    }

    return(k_eExecutionContextOther);
#else
// this can be implemented using TaskLevel if you don't mind linking against
//    DebugLib (and therefore breaking Mac OS 8.6 support), or CurrentExecutionLevel.
#    error execution_context unimplimented
#endif
}


} // namespace mac

} // namespace threads

} // namespace boost
