// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.  
// It is provided "as is" without express or implied warranty.

#include <boost/thread/tss.hpp>
#include <stdexcept>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
tss::tss()
{
    m_key = TlsAlloc();
    assert(m_key != 0xFFFFFFFF);

    if (m_key == 0xFFFFFFFF)
        throw std::runtime_error("boost::tss : failure to construct");
}

tss::~tss()
{
    int res = TlsFree(m_key);
    assert(res);
}

void* tss::get() const
{
    return TlsGetValue(m_key);
}

bool tss::set(void* value)
{
    return TlsSetValue(m_key, value);
}
#elif defined(BOOST_HAS_PTHREADS)
tss::tss()
{
    int res = pthread_key_create(&m_key, 0);
    assert(res == 0);

    if (res != 0)
        throw std::runtime_error("boost::tss : failure to construct");
}

tss::~tss()
{
    int res = pthread_key_delete(m_key);
    assert(res == 0);
}

void* tss::get() const
{
    return pthread_getspecific(m_key);
}

bool tss::set(void* value)
{
    return pthread_setspecific(m_key, value) == 0;
}
#endif

} // namespace boost

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
