// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_DELIVERY_MAN_MJM012402_HPP
#define BOOST_DELIVERY_MAN_MJM012402_HPP


#include <boost/function.hpp>
#include <boost/utility.hpp>

#include <boost/thread/mutex.hpp>

#include "package.hpp"

#include <Multiprocessing.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// class delivery_man is intended to move boost::function objects from MP tasks to
//    other execution contexts (such as deferred task time or system task time).

class delivery_man: private noncopyable
{
  public:
    delivery_man();
    ~delivery_man();

  public:
    template<class R>
    R deliver(function<R> &rFunctor);

    void accept_deliveries();

  private:
    base_package *m_pPackage;
    mutex m_oMutex;
    MPSemaphoreID m_pSemaphore;
    bool m_bPackageWaiting;
};


template<class R>
R delivery_man::deliver(function<R> &rFunctor)
{
    assert(at_mp());

// lock our mutex
    mutex::scoped_lock oLock(m_oMutex);

// create a package and save it
    package<R> oPackage(rFunctor);
    m_pPackage = &oPackage;
    m_bPackageWaiting = true;

// wait on the semaphore
    OSStatus lStatus = MPWaitOnSemaphore(m_pSemaphore, kDurationForever);
    assert(lStatus == noErr);

    return(oPackage.return_value());
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_DELIVERY_MAN_MJM012402_HPP
