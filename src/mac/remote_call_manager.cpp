// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "remote_call_manager.hpp"

#include <boost/bind.hpp>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


using detail::delivery_man;


remote_call_manager::remote_call_manager():
    m_oDTDeliveryMan(),
    m_oSTDeliveryMan(),
    m_oDTFunction(bind(&delivery_man::accept_deliveries, &m_oDTDeliveryMan)),
    m_oSTFunction(bind(&delivery_man::accept_deliveries, &m_oSTDeliveryMan)),
    m_oDTPeriodical(m_oDTFunction),
    m_oSTPeriodical(m_oSTFunction)
{
    m_oDTPeriodical.start();
    m_oSTPeriodical.start();
}

remote_call_manager::~remote_call_manager()
{
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
