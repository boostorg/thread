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
#include <boost/thread/mutex.hpp>
#include <boost/thread/exceptions.hpp>
#include <vector>
#include <stdexcept>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

#if defined(BOOST_HAS_WINTHREADS)
#include "threadmon.hpp"
namespace {
	typedef std::vector<std::pair<boost::detail::tss*, int> > key_type;
	typedef std::vector<void*> slots_type;

	DWORD key;
	boost::once_flag once = BOOST_ONCE_INIT;
	boost::mutex* pmutex;
	key_type* pkeys;
	int next_key;

	void __cdecl cleanup_tss_data();

	void init_tss()
	{
		static boost::mutex mutex;
		static key_type keys;
		pmutex = &mutex;
		pkeys = &keys;
		key = TlsAlloc();
		assert(key != 0xFFFFFFFF);
		next_key = 0;
	}

	int alloc_key(boost::detail::tss* ptss)
	{
		boost::call_once(&init_tss, once);
		boost::mutex::scoped_lock lock(*pmutex);
		int key = next_key;
		if (key >= pkeys->size())
		{
			pkeys->resize(key+1);
			(*pkeys)[key].second = pkeys->size();
		}
		next_key = (*pkeys)[key].second;
		(*pkeys)[key].first = ptss;
		return key;
	}

	void free_key(int key)
	{
		boost::call_once(&init_tss, once);
		boost::mutex::scoped_lock lock(*pmutex);
		assert(key >= 0 && key < pkeys->size());
		(*pkeys)[key].first = 0;
		(*pkeys)[key].second = next_key;
		next_key = key;
	}

	slots_type* get_tss_data()
	{
		boost::call_once(&init_tss, once);
		if (key == 0xFFFFFFFF)
			return 0;
		slots_type* pdata = (slots_type*)TlsGetValue(key);
		if (pdata == 0)
		{
			std::auto_ptr<slots_type> slots(new(std::nothrow) slots_type);
			if (!TlsSetValue(key, slots.get()))
				return 0;
			on_thread_exit(&cleanup_tss_data);
			pdata = slots.release();
		}
		return pdata;
	}

	void __cdecl cleanup_tss_data()
	{
		slots_type* pdata = get_tss_data();
		if (pdata)
		{
			boost::mutex::scoped_lock lock(*pmutex);
			for (int key = 0; key < pdata->size(); ++key)
			{
				void* pvalue = (*pdata)[key];
				boost::detail::tss* ptss = pkeys && key < pkeys->size() ? (*pkeys)[key].first : 0;

				if (ptss && pvalue)
					ptss->cleanup(pvalue);
			}
			delete pdata;
		}
	}
}
#elif defined(BOOST_HAS_MPTASKS)
#include <map>
namespace {
    typedef std::pair<void(*)(void*), void*> cleanup_info;
    typedef std::map<int, cleanup_info> cleanup_handlers;

    TaskStorageIndex key;
    boost::once_flag once = BOOST_ONCE_INIT;

    void init_cleanup_key()
    {
        OSStatus lStatus = MPAllocateTaskStorageIndex(&key);
        assert(lStatus == noErr);
    }

    cleanup_handlers* get_handlers()
    {
        boost::call_once(&init_cleanup_key, once);

        cleanup_handlers* handlers = reinterpret_cast<cleanup_handlers*>(MPGetTaskStorageValue(key));
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
            OSStatus lStatus = noErr;
            lStatus = MPSetTaskStorageValue(key, reinterpret_cast<TaskStorageValue>(handlers));
            assert(lStatus == noErr);
        // TODO - create a generalized mechanism for registering thread exit functions
        //            and use it here.
        }

        return handlers;
    }
}

namespace boost {

namespace detail {

void thread_cleanup()
{
    cleanup_handlers* handlers = reinterpret_cast<cleanup_handlers*>(MPGetTaskStorageValue(key));
    if(handlers != NULL)
    {
        for (cleanup_handlers::iterator it = handlers->begin(); it != handlers->end(); ++it)
        {
            cleanup_info info = it->second;
            if (info.second)
                info.first(info.second);
        }
        delete handlers;
    }
}


} // namespace detail

} // namespace boost

#endif

namespace boost { namespace detail {

#if defined(BOOST_HAS_WINTHREADS)
tss::tss(boost::function1<void, void*> cleanup)
{
	m_key = alloc_key(this);
	m_clean = cleanup;
	m_module = (void*)LoadLibrary("boostthreadmon.dll");
}

tss::~tss()
{
	free_key(m_key);
	FreeLibrary((HMODULE)m_module);
}

void* tss::get() const
{
	slots_type* pdata = get_tss_data();
	if (pdata)
	{
		if (m_key >= pdata->size())
			return 0;
		return (*pdata)[m_key];
	}
	return 0;
}

void tss::set(void* value)
{
	slots_type* pdata = get_tss_data();
	if (!pdata)
		throw thread_resource_error();
	if (m_key >= pdata->size())
	{
		try
		{
			pdata->resize(m_key+1);
		}
		catch (...)
		{
			throw thread_resource_error();
		}
	}
	(*pdata)[m_key] = value;
}

void tss::cleanup(void* value)
{
	m_clean(value);
}
#elif defined(BOOST_HAS_PTHREADS)
tss::tss(void (*cleanup)(void*))
{
    int res = 0;
    res = pthread_key_create(&m_key, cleanup);
    if (res != 0)
        throw thread_resource_error();
}

tss::~tss()
{
    int res = 0;
    res = pthread_key_delete(m_key);
    assert(res == 0);
}

void* tss::get() const
{
    return pthread_getspecific(m_key);
}

void tss::set(void* value)
{
	int res = pthread_setspecific(m_key, value) == 0;
	assert(res == 0 || res = ENOMEM);
	if (res == ENOMEM)
		throw thread_resource_error();
}
#elif defined(BOOST_HAS_MPTASKS)
tss::tss(void (*cleanup)(void*))
{
    OSStatus lStatus = MPAllocateTaskStorageIndex(&m_key);
    if(lStatus != noErr)
        throw thread_resource_error();

    m_cleanup = cleanup;
}

tss::~tss()
{
    OSStatus lStatus = MPDeallocateTaskStorageIndex(m_key);
    assert(lStatus == noErr);
}

void* tss::get() const
{
    TaskStorageValue ulValue = MPGetTaskStorageValue(m_key);
    return(reinterpret_cast<void *>(ulValue));
}

void tss::set(void* value)
{
    if (m_cleanup)
    {
        cleanup_handlers* handlers = get_handlers();
        assert(handlers);
        if (!handlers)
            return false;
        cleanup_info info(m_cleanup, value);
        (*handlers)[m_key] = info;
    }
    OSStatus lStatus = MPSetTaskStorageValue(m_key, reinterpret_cast<TaskStorageValue>(value));
//    return(lStatus == noErr);
}
#endif

} // namespace detail
} // namespace boost

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
//  30 May 02  WEKEMPF Added interface to set specific cleanup handlers. Removed TLS slot limits
//                     from most implementations.
