// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_SCOPED_CRITICAL_REGION_MJM012402_HPP
#define BOOST_SCOPED_CRITICAL_REGION_MJM012402_HPP


#include <boost/thread/exceptions.hpp>

#include <Multiprocessing.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// class scoped_critical_region probably needs a new name.  Although the current name
//    is accurate, it can be read to mean that a critical region is entered for the
//    current scope.  In reality, a critical region is _created_ for the current scope.
//    This class is intended as a replacement for MPCriticalRegionID that will
//    automatically create and dispose of itself.

class scoped_critical_region
{
  public:
    scoped_critical_region();
    ~scoped_critical_region();

  public:
    operator const MPCriticalRegionID &() const;
    const MPCriticalRegionID &get() const;

  private:
    MPCriticalRegionID m_pCriticalRegionID;
};


// these are inlined for speed.
inline scoped_critical_region::operator const MPCriticalRegionID &() const
    {    return(m_pCriticalRegionID);    }
inline const MPCriticalRegionID &scoped_critical_region::get() const
    {    return(m_pCriticalRegionID);    }


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_SCOPED_CRITICAL_REGION_MJM012402_HPP
