// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2006 Roland Schwarz
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/win32/config.hpp>

#include <boost/thread/win32/tss.hpp>
#ifndef BOOST_THREAD_NO_TSS_CLEANUP

#include <boost/thread/win32/once.hpp>
#include <boost/thread/win32/mutex.hpp>
#include <boost/thread/win32/exceptions.hpp>
#include <vector>
#include <string>
#include <stdexcept>
#include <cassert>

#include <windows.h>
#include <boost/thread/win32/tss_hooks.hpp>

namespace {

typedef std::vector<void*> tss_slots;

struct tss_data_t
{
    boost::mutex mutex;
    std::vector<boost::function1<void, void*>*> cleanup_handlers;
    DWORD native_key;
};

tss_data_t* tss_data = 0;
boost::once_flag tss_data_once = BOOST_ONCE_INIT;

extern "C" void cleanup_slots(void* p)
{
    tss_slots* slots = static_cast<tss_slots*>(p);
    for (tss_slots::size_type i = 0; i < slots->size(); ++i)
    {
        boost::mutex::scoped_lock lock(tss_data->mutex);
        (*tss_data->cleanup_handlers[i])((*slots)[i]);
        (*slots)[i] = 0;
    }
    delete slots;
}

void init_tss_data()
{
    std::auto_ptr<tss_data_t> temp(new tss_data_t);

    //Force the cleanup implementation library to be linked in
    tss_cleanup_implemented();

    //Allocate tls slot
    temp->native_key = TlsAlloc();
    if (temp->native_key == 0xFFFFFFFF)
        return;

    // Intentional memory "leak"
    // This is the only way to ensure the mutex in the global data
    // structure is available when cleanup handlers are run, since the
    // execution order of cleanup handlers is unspecified on any platform
    // with regards to C++ destructor ordering rules.
    tss_data = temp.release();
}

tss_slots* get_slots(bool alloc);

void __cdecl tss_thread_exit()
{
    tss_slots* slots = get_slots(false);
    if (slots)
        cleanup_slots(slots);
}

tss_slots* get_slots(bool alloc)
{
    tss_slots* slots = 0;

    slots = static_cast<tss_slots*>(
        TlsGetValue(tss_data->native_key));

    if (slots == 0 && alloc)
    {
        std::auto_ptr<tss_slots> temp(new tss_slots);

        if (at_thread_exit(&tss_thread_exit) == -1)
            return 0;
        if (!TlsSetValue(tss_data->native_key, temp.get()))
            return 0;

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

#endif //BOOST_THREAD_NO_TSS_CLEANUP
