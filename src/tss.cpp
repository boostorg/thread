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
    typedef std::pair<void(*)(void*), void*> cleanup_info;
    typedef std::pair<int, cleanup_info> cleanup_node;
    struct cleanup_info_less
    {
		bool operator()(const cleanup_node& x, const cleanup_node& y) const
		{
			return x.first < y.first;
		}
    };
    typedef std::set<cleanup_node, cleanup_info_less> cleanup_handlers;

    DWORD key;
    boost::once_flag once = boost::once_init;

    void init_cleanup_key()
    {
        key = TlsAlloc();
        assert(key != 0xFFFFFFFF);
    }

    void __cdecl cleanup()
    {
        cleanup_handlers* handlers = static_cast<cleanup_handlers*>(TlsGetValue(key));
        for (cleanup_handlers::iterator it = handlers->begin(); it != handlers->end(); ++it)
        {
            cleanup_node node = *it;
            cleanup_info info = node.second;
            if (info.second)
                info.first(info.second);
        }
        delete handlers;
    }

    cleanup_handlers* get_handlers()
    {
        boost::call_once(&init_cleanup_key, once);

        cleanup_handlers* handlers = static_cast<cleanup_handlers*>(TlsGetValue(key));
        if (!handlers)
        {
            try
            {
                handlers = new cleanup_handlers;
            }
            catch (...)
            {
                return 0;
            }
            int res = TlsSetValue(key, handlers);
            assert(res);
            res = on_thread_exit(&cleanup);
            assert(res == 0);
        }

        return handlers;
    }
}
#endif

namespace boost { namespace detail {

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
        cleanup_info info(m_cleanup, value);
        cleanup_node node(m_key, info);
        cleanup_handlers::iterator it = handlers->lower_bound(node);
        if (it == handlers->end())
        {
            if (!handlers->insert(node).second)
                return false;
        }
        else
            (*it).second = info;
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

} // namespace detail
} // namespace boost

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
