//  thread_primitives.cpp
//
//  (C) Copyright 2018 Andrey Semashev
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/winapi/config.hpp>
#include <boost/winapi/dll.hpp>
#include <boost/winapi/time.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/memory_order.hpp>
#include <boost/atomic/atomic.hpp>
#include <boost/thread/win32/interlocked_read.hpp>
#include <boost/thread/win32/thread_primitives.hpp>

namespace boost {
namespace detail {
namespace win32 {

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6

// Directly use API from Vista and later
BOOST_THREAD_DECL boost::detail::win32::detail::gettickcount64_t gettickcount64 = &::boost::winapi::GetTickCount64;

#else // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6

namespace {

// Zero-initialized initially
BOOST_ALIGNMENT(64) static boost::atomic< uint64_t > g_ticks;

//! Artifical implementation of GetTickCount64
ticks_type WINAPI get_tick_count64()
{
    uint64_t old_state = g_ticks.load(boost::memory_order_acquire);

    uint32_t new_ticks = boost::winapi::GetTickCount();

    uint32_t old_ticks = static_cast< uint32_t >(old_state & UINT64_C(0x00000000ffffffff));
    uint64_t new_state = ((old_state & UINT64_C(0xffffffff00000000)) + (static_cast< uint64_t >(new_ticks < old_ticks) << 32)) | static_cast< uint64_t >(new_ticks);

    g_ticks.store(new_state, boost::memory_order_release);

    return new_state;
}

ticks_type WINAPI get_tick_count_init()
{
    boost::winapi::HMODULE_ hKernel32 = boost::winapi::GetModuleHandleW(L"kernel32.dll");
    if (hKernel32)
    {
        boost::detail::win32::detail::gettickcount64_t p =
            (boost::detail::win32::detail::gettickcount64_t)boost::winapi::get_proc_address(hKernel32, "GetTickCount64");
        if (p)
        {
            // Use native API
            boost::detail::interlocked_write_release(&gettickcount64, p);
            return p();
        }
    }

    // No native API available
    boost::detail::interlocked_write_release(&gettickcount64, &get_tick_count64);
    return get_tick_count64();
}

} // namespace

BOOST_THREAD_DECL boost::detail::win32::detail::gettickcount64_t gettickcount64 = &get_tick_count_init;

#endif // BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6

} // namespace win32
} // namespace detail
} // namespace boost

