// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "thread_cleanup.hpp"


namespace boost {

namespace threads {

namespace mac {

namespace detail {


namespace {

TaskStorageIndex g_ulIndex(0UL);

} // anonymous namespace


void do_thread_startup()
{
    if(g_ulIndex == 0UL)
    {
        OSStatus lStatus = MPAllocateTaskStorageIndex(&g_ulIndex);
        assert(lStatus == noErr);
    }
    set_thread_cleanup_task(NULL);
}

void do_thread_cleanup()
{
    void (*pfnTask)() = MPGetTaskValue(g_ulIndex)
}


void set_thread_cleanup_task(void (*pfnTask)())
{
    lStatus = MPSetTaskValue(g_ulIndex, reinterpret_cast<TaskStorageValue>(pfnTask));
    assert(lStatus == noErr);
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
