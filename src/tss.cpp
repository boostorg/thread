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
#	include "threadmon.hpp"
#endif

namespace {
	typedef std::vector<void*> tss_slots;

	struct tss_slot_info
	{
		boost::function1<void, void*> cleanup;
		void* main_data;
		int next;
		int prev;
	};
	typedef std::vector<tss_slot_info> tss_slot_vector;

	struct tss_data_t
	{
		boost::mutex mutex;
		tss_slot_vector slots;
#if defined(BOOST_HAS_WINTHREADS)
		DWORD main_thread;
		DWORD native_key;
#endif
		int next_free;
		int next_in_use;
	};

	tss_data_t* tss_data = 0;
	boost::once_flag tss_once = BOOST_ONCE_INIT;

	void init_tss_data()
	{
		static tss_data_t instance;
		tss_data = &instance;
#if defined(BOOST_HAS_WINTHREADS)
		instance.main_thread = GetCurrentThreadId();
		instance.native_key = TlsAlloc();
#endif
		instance.next_free = instance.next_in_use = -1;
	}

	bool is_main_thread()
	{
#if defined(BOOST_HAS_WINTHREADS)
		return GetCurrentThreadId() == tss_data->main_thread;
#endif
		return false;
	}

	void cleanup_slots(void* p)
	{
		tss_slots* slots = static_cast<tss_slots*>(p);
		boost::mutex::scoped_lock lock(tss_data->mutex);
		int i = tss_data->next_in_use;
		while (i != -1)
		{
			if (i < slots->size() && (*slots)[i] != 0)
			{
				tss_data->slots[i].cleanup((*slots)[i]);
				(*slots)[i] = 0;
			}
			i = tss_data->slots[i].next;
		}
	}

#if defined(BOOST_HAS_WINTHREADS)
	void __cdecl tss_thread_exit()
	{
		tss_slots* slots = static_cast<tss_slots*>(TlsGetValue(tss_data->native_key));
		cleanup_slots(slots);
	}
#endif

	tss_slots* get_slots(bool alloc)
	{
		tss_slots* slots = 0;

#if defined(BOOST_HAS_WINTHREADS)
		slots = static_cast<tss_slots*>(TlsGetValue(tss_data->native_key));
#endif

		if (slots == 0 && alloc)
		{
			std::auto_ptr<tss_slots> temp(new tss_slots);

#if defined(BOOST_HAS_WINTHREADS)
			if (!TlsSetValue(tss_data->native_key, temp.get()))
				return 0;
			on_thread_exit(&tss_thread_exit);
#else
			return 0;
#endif

			slots = temp.release();
		}

		return slots;
	}
} // namespace 

namespace boost {

	namespace detail {
		tss_ref::tss_ref()
		{
			boost::call_once(&init_tss_data, tss_once);
		}

		tss::tss(boost::function1<void, void*> cleanup)
		{
			boost::mutex::scoped_lock lock(tss_data->mutex);
			m_slot = tss_data->next_free;
			if (m_slot == -1)
			{
				tss_slot_info info;
				info.next = -1;
				info.prev = -1;
				try
				{
					tss_data->slots.push_back(info);
				}
				catch (...)
				{
					throw boost::thread_resource_error();
				}
				m_slot = tss_data->slots.size() - 1;
			}
			tss_data->next_free = tss_data->slots[m_slot].next;
			tss_data->slots[m_slot].next = tss_data->next_in_use;
			if (tss_data->next_in_use != -1)
				tss_data->slots[tss_data->next_in_use].prev = m_slot;
			tss_data->next_in_use = m_slot;
			tss_data->slots[m_slot].prev = -1;
			tss_data->slots[m_slot].cleanup = cleanup;
			tss_data->slots[m_slot].main_data = 0;
		}

		tss::~tss()
		{
			boost::mutex::scoped_lock lock(tss_data->mutex);
			if (tss_data->slots[m_slot].main_data)
				tss_data->slots[m_slot].cleanup(tss_data->slots[m_slot].main_data);
			if (tss_data->slots[m_slot].prev != -1)
				tss_data->slots[tss_data->slots[m_slot].prev].next = tss_data->slots[m_slot].next;
			if (tss_data->slots[m_slot].next != -1)
				tss_data->slots[tss_data->slots[m_slot].next].prev = tss_data->slots[m_slot].prev;
			tss_data->slots[m_slot].next = tss_data->next_free;
			tss_data->next_free = m_slot;
		}

		void* tss::get() const
		{
			if (is_main_thread())
			{
				boost::mutex::scoped_lock lock(tss_data->mutex);
				return tss_data->slots[m_slot].main_data;
			}

			tss_slots* slots = get_slots(false);

			if (!slots)
				return 0;

			if (m_slot >= slots->size())
				return 0;

			return (*slots)[m_slot];
		}
		
		void tss::set(void* value)
		{
			if (is_main_thread())
			{
				boost::mutex::scoped_lock lock(tss_data->mutex);
				tss_data->slots[m_slot].main_data = value;
			}
			else
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
		}

		void tss::cleanup(void* value)
		{
			boost::mutex::scoped_lock lock(tss_data->mutex);
			tss_data->slots[m_slot].cleanup(value);
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
//  30 May 02  WEKEMPF Added interface to set specific cleanup handlers. Removed TLS slot limits
//                     from most implementations.
