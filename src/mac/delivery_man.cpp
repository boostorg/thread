// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "delivery_man.hpp"

#include "os.hpp"
#include "execution_context.hpp"


namespace boost {

namespace threads {

namespace mac {

namespace detail {


delivery_man::delivery_man():
    m_pPackage(NULL),
    m_pSemaphore(kInvalidID),
    m_bPackageWaiting(false)
{
    assert(at_st());

    OSStatus lStatus = MPCreateSemaphore(1UL, 0UL, &m_pSemaphore);
// TODO - throw on error here
    assert(lStatus == noErr);
}

delivery_man::~delivery_man()
{
    assert(m_bPackageWaiting == false);

    OSStatus lStatus = MPDeleteSemaphore(m_pSemaphore);
    assert(lStatus == noErr);
}


void delivery_man::accept_deliveries()
{
    if(m_bPackageWaiting)
    {
        assert(m_pPackage != NULL);
        m_pPackage->accept();
        m_pPackage = NULL;
        m_bPackageWaiting = false;

    // signal to the thread making the call that we're done
        OSStatus lStatus = MPSignalSemaphore(m_pSemaphore);
        assert(lStatus == noErr);
    }
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
