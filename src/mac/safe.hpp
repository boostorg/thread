// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_SAFE_MJM012402_HPP
#define BOOST_SAFE_MJM012402_HPP


#include <Multiprocessing.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// these functions are used to wain in an execution context-independent manor.  All of these
//  functions are both MP- and ST-safe.

OSStatus safe_wait_on_semaphore(MPSemaphoreID pSemaphoreID, Duration lDuration);
OSStatus safe_enter_critical_region(MPCriticalRegionID pCriticalRegionID, Duration lDuration, MPCriticalRegionID pCriticalRegionCriticalRegionID = kInvalidID);
OSStatus safe_wait_on_queue(MPQueueID pQueueID, void **pParam1, void **pParam2, void **pParam3, Duration lDuration);
OSStatus safe_delay_until(AbsoluteTime *pWakeUpTime);


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost

#endif // BOOST_SAFE_MJM012402_HPP
