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
#	include <boost/thread/detail/threadmon.hpp>
#endif

namespace {

typedef std::vector<void*> tss_slots;

struct tss_data_t
{
    boost::mutex mutex;
	std::vector<boost::function1<void, void*>*> cleanup_handlers;
#if defined(BOOST_HAS_WINTHREADS)
    DWORD native_key;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_key_t native_key;
#endif
};

tss_data_t* tss_data = 0;
boost::once_flag tss_data_once = BOOST_ONCE_INIT;

void init_tss_data()
{
	std::auto_ptr<tss_data_t> temp(new tss_data_t);

#if defined(BOOST_HAS_WINTHREADS)
    temp->native_key = TlsAlloc();
	if (temp->native_key == 0xFFFFFFFF)
		return;
#elif defined(BOOST_HAS_PTHREADS)
    int res = 0;
    res = pthread_key_create(&temp->native_key, &cleanup_slots);
	if (res != 0)
		return;
#endif

    // Intentional memory "leak"
    // This is the only way to ensure the mutex in the global data
    // structure is available when cleanup handlers are run, since the
    // execution order of cleanup handlers is unspecified on any platform
    // with regards to C++ destructor ordering rules.
	tss_data = temp.release();
}

extern "C" void cleanup_slots(void* p)
{
    tss_slots* slots = static_cast<tss_slots*>(p);
	for (tss_slots::size_type i = 0; i < slots->size(); ++i)
    {
	    boost::mutex::scoped_lock lock(tss_data->mutex);
		(*tss_data->cleanup_handlers[i])((*slots)[i]);
		(*slots)[i] = 0;
    }
}

#if defined(BOOST_HAS_WINTHREADS)
void __cdecl tss_thread_exit()
{
    tss_slots* slots = static_cast<tss_slots*>(
        TlsGetValue(tss_data->native_key));
    if (slots)
        cleanup_slots(slots);
}
#endif

tss_slots* get_slots(bool alloc)
{
    tss_slots* slots = 0;

#if defined(BOOST_HAS_WINTHREADS)
    slots = static_cast<tss_slots*>(TlsGetValue(tss_data->native_key));
#elif defined(BOOST_HAS_PTHREADS)
    slots = static_cast<tss_slots*>(
        pthread_getspecific(tss_data->native_key));
#endif

    if (slots == 0 && alloc)
    {
        std::auto_ptr<tss_slots> temp(new tss_slots);

#if defined(BOOST_HAS_WINTHREADS)
        if (on_thread_exit(&tss_thread_exit) == -1)
			return 0;
        if (!TlsSetValue(tss_data->native_key, temp.get()))
            return 0;
#elif defined(BOOST_HAS_PTHREADS)
        if (pthread_setspecific(tss_data->native_key, temp.get()) != 0)
            return 0;
#endif

        slots = temp.release();
    }

    return slots;
}

} // namespace 

namespace boost {

namespace detail {
void tss::init(boost::function1<void, void*>* pcleanup)
{
    boost::call_once(&init_tss_data, tss_data_once);
	if (tss_data == 0)
		throw thread_resource_error();
    boost::mutex::scoped_lock lock(tss_data->mutex);
	try
	{
		tss_data->cleanup_handlers.push_back(pcleanup);
		m_slot = tss_data->cleanup_handlers.size() - 1;
	}
	catch (...)
	{
		throw thread_resource_error();
	}
}

void* tss::get() const
{
    tss_slots* slots = get_slots(false);

    if (!slots)
        return 0;

    if (m_slot >= slots->size())
        return 0;

    return (*slots)[m_slot];
}
	
void tss::set(void* value)
{
    tss_slots* slots = get_slots(true);

    if (!slots)
        throw boost::thread_resource_error();

    if (m_slot >= slots->size())
    {
        try
        {
            slots->resize(m_slot + 1);
        }
        catch (...)
        {
            throw boost::thread_resource_error();
        }
    }

    (*slots)[m_slot] = value;
}

void tss::cleanup(void* value)
{
    boost::mutex::scoped_lock lock(tss_data->mutex);
	(*tss_data->cleanup_handlers[m_slot])(value);
}

} // namespace detail

} // namespace boost

/*
#if defined(BOOST_HAS_WINTHREADS)
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
//		static boost::mutex mutex;
//		static key_type keys;
//		pmutex = &mutex;
//		pkeys = &keys;
		pmutex = new boost::mutex;
		pkeys = new key_type;
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
*/

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
//  30 May 02  WEKEMPF Added interface to set specific cleanup handlers.
//                     Removed TLS slot limits from most implementations.
