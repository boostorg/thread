// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_THREADMON_WEK062504_HPP
#define BOOST_THREADMON_WEK062504_HPP

#include <boost/thread/detail/config.hpp>

#ifdef BOOST_HAS_WINTHREADS

extern "C" BOOST_THREAD_DECL int at_thread_exit(void (__cdecl * func)(void));
    //Add a function to the list of thread-exit functions

extern "C" BOOST_THREAD_DECL void on_process_enter(void);
    //To be called when the process starts, when the dll is loaded, etc.
    //Called automatically by Boost.Thread when possible
extern "C" BOOST_THREAD_DECL void on_thread_exit(void);
    //To be called for each thread when it exits
    //Must be called in the context of the thread that is exiting
    //Called automatically by Boost.Thread when possible
extern "C" BOOST_THREAD_DECL void on_process_exit(void);
    //To be called when the process exits, when the dll is unloaded, etc.
    //Called automatically by Boost.Thread when possible

#endif // BOOST_HAS_WINTHREADS

#endif // BOOST_THREADMON_WEK062504_HPP
