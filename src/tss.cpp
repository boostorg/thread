/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *   6 Jun 01  Initial version.
 */
 
#include <boost/thread/tss.hpp>
#include <stdexcept>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

#include <cassert>

namespace boost
{
#if defined(BOOST_HAS_WINTHREADS)
    tss::tss()
    {
        _key = TlsAlloc();
        assert(_key != 0xFFFFFFFF);

        if (_key == 0xFFFFFFFF)
            throw std::runtime_error("boost::tss : failure to construct");
    }

    tss::~tss()
    {
        int res = TlsFree(_key);
        assert(res);
    }

    void* tss::get() const
    {
        return TlsGetValue(_key);
    }

    bool tss::set(void* value)
    {
        return TlsSetValue(_key, value);
    }
#elif defined(BOOST_HAS_PTHREADS)
    tss::tss()
    {
        int res = pthread_key_create(&_key, 0);
        assert(res == 0);

        if (res != 0)
            throw std::runtime_error("boost::tss : failure to construct");
    }

    tss::~tss()
    {
        int res = pthread_key_delete(_key);
        assert(res == 0);
    }

    void* tss::get() const
    {
        return pthread_getspecific(_key);
    }

    bool tss::set(void* value)
    {
        return pthread_setspecific(_key, value) == 0;
    }
#endif
}