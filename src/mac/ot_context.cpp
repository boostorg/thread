// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "ot_context.hpp"

#include "execution_context.hpp"


#include <cassert>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


ot_context::ot_context()
{
    assert(at_st());

    OSStatus lStatus = InitOpenTransportInContext(0UL, &m_pContext);
// TODO - throw on error
    assert(lStatus == noErr);
}

ot_context::~ot_context()
{
    CloseOpenTransportInContext(m_pContext);
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
