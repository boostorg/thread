// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_OT_CONTEXT_MJM012402_HPP
#define BOOST_OT_CONTEXT_MJM012402_HPP


#include <OpenTransport.h>

#include <boost/utility.hpp>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// class ot_context is intended to be used only as a singleton.  All that this class
//    does is ask OpenTransport to create him an OTClientContextPtr, and then doles
//    this out to anyone who wants it.  ot_context should only be instantiated at
//    system task time.

class ot_context: private noncopyable
{
  protected:
    ot_context();
    ~ot_context();

  public:
    OTClientContextPtr get_context();

  private:
    OTClientContextPtr m_pContext;
};


inline OTClientContextPtr ot_context::get_context()
    {    return(m_pContext);    }


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_OT_CONTEXT_MJM012402_HPP
