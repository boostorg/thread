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
#include <boost/thread/once.hpp>
#include "threadmon.hpp"
#include <stdexcept>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

#if defined(BOOST_HAS_WINTHREADS)
#include <set>
namespace {
    typedef std::pair<boost::tss*,void(*)(void*)> cleanup_info;
    struct cleanup_info_less
    {
		bool operator()(const cleanup_info& x, const cleanup_info& y) const
		{
			return x.first < y.first;
		}
    };
    typedef std::set<cleanup_info, cleanup_info_less> cleanup_handlers;

    DWORD key;

    void init_cleanup_key()
    {
        key = TlsAlloc();
    }

    void __cdecl cleanup()
    {
        cleanup_handlers* handlers = static_cast<cleanup_handlers*>(TlsGetValue(key));
        for (cleanup_handlers::iterator it = handlers->begin(); it != handlers->end(); ++it)
        {
            cleanup_info info = *it;
            void* ptr = info.first->get();
            if (ptr)
                info.second(ptr);
        }
        delete handlers;
    }

    cleanup_handlers* get_handlers()
    {
        static boost::once_flag once = BOOST_ONCE_INIT;
        boost::call_once(&init_cleanup_key, once);

        cleanup_handlers* handlers = static_cast<cleanup_handlers*>(TlsGetValue(key));
        if (!handlers)
        {
            handlers = new cleanup_handlers;
            TlsSetValue(key, handlers);
            on_thread_exit(&cleanup);
        }

        return handlers;
    }
}
#endif

namespace boost {

#if defined(BOOST_HAS_WINTHREADS)
tss::tss(void (*cleanup)(void*))
{
    m_key = TlsAlloc();
    assert(m_key != 0xFFFFFFFF);

    if (m_key == 0xFFFFFFFF)
        throw std::runtime_error("boost::tss : failure to construct");

    m_cleanup = cleanup;
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
    if (value && m_cleanup)
    {
        cleanup_handlers* handlers = get_handlers();
        assert(handlers);
        if (!handlers)
            return false;
        cleanup_info info(this, m_cleanup);
        cleanup_handlers::iterator it = handlers->lower_bound(info);
        if (it == handlers->end())
            handlers->insert(cleanup_info(this, m_cleanup));
    }
    return TlsSetValue(m_key, value);
}
#elif defined(BOOST_HAS_PTHREADS)
tss::tss(void (*cleanup)(void*))
{
    int res = pthread_key_create(&m_key, cleanup);
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
