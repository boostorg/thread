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
        //that are called by on_thread_exit() and friends.

extern "C" BOOST_THREAD_DECL void on_process_enter(void);
    //To be called once when the process starts (or, if a dll
        //once when the dll is loaded into the process).
    //Called automatically by Boost.Thread when possible.
    //Can be omitted; however the process may be more efficient
        //if it is not. If called should be called only
        //once and must be balanced by a call to on_process_exit().

extern "C" BOOST_THREAD_DECL void on_process_exit(void);
    //To be called once when the process ends (or, if a dll
        //once when the dll is unloaded from the process).
    //Called automatically by Boost.Thread when possible.
    //Can be omitted; however the process may be more efficient
        //if it is not. If called should be called only
        //once and must be balanced by a call to on_process_enter().

extern "C" BOOST_THREAD_DECL void on_thread_enter(void);
    //To be called for each thread when it starts.
    //Called automatically by Boost.Thread when possible.
    //Must be called in the context of the thread that is entering.
    //Can be called multiple times per thread.

extern "C" BOOST_THREAD_DECL void on_thread_exit(void);
    //To be called for each thread when it exits.
    //Called automatically by Boost.Thread when possible.
    //Must be called in the context of the thread that is exiting.
    //Can be called multiple times per thread.

#endif // BOOST_HAS_WINTHREADS

#endif // BOOST_THREADMON_WEK062504_HPP
