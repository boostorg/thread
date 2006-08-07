// (C) Copyright Michael Glassford 2004.
// Copyright (c) 2006 Peter Dimov
// Copyright (c) 2006 Anthony Williams
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>

#if defined(BOOST_HAS_WINTHREADS)

#include <boost/thread/detail/tss_hooks.hpp>

#include <boost/assert.hpp>
#include <boost/thread/once.hpp>

#include <list>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{

typedef std::list<thread_exit_handler> thread_exit_handlers;

const DWORD invalid_tls_key = TLS_OUT_OF_INDEXES;
DWORD tls_key = invalid_tls_key;

boost::once_flag once_init_tls_key = BOOST_ONCE_INIT;

void init_tls_key()
{
    tls_key = TlsAlloc();
}

} // unnamed namespace

extern "C" BOOST_THREAD_DECL int at_thread_exit( thread_exit_handler exit_handler )
{
    boost::call_once( init_tls_key, once_init_tls_key );

    if( tls_key == invalid_tls_key )
    {
        return -1;
    }

    // Get the exit handlers list for the current thread from tls.

    thread_exit_handlers* exit_handlers =
        static_cast< thread_exit_handlers* >( TlsGetValue( tls_key ) );

    if( exit_handlers == 0 )
    {
        // No exit handlers list was created yet.

        try
        {
            // Attempt to create a new exit handlers list.

            exit_handlers = new thread_exit_handlers;

            if( exit_handlers == 0 )
            {
                return -1;
            }

            // Attempt to store the list pointer in tls.

            if( !TlsSetValue( tls_key, exit_handlers ) )
            {
                delete exit_handlers;
                return -1;
            }
        }
        catch( ... )
        {
            return -1;
        }
    }

    // Like the C runtime library atexit() function,
    // functions should be called in the reverse of
    // the order they are added, so push them on the
    // front of the list.

    try
    {
        exit_handlers->push_front( exit_handler );
    }
    catch( ... )
    {
        return -1;
    }

    // Like the atexit() function, a result of zero
    // indicates success.

    return 0;
}

extern "C" BOOST_THREAD_DECL void on_process_enter()
{
}

extern "C" BOOST_THREAD_DECL void on_process_exit()
{
    if( tls_key != invalid_tls_key )
    {
        TlsFree(tls_key);
    }
}

extern "C" BOOST_THREAD_DECL void on_thread_enter()
{
}

extern "C" BOOST_THREAD_DECL void on_thread_exit()
{
    // Initializing tls_key here ensures its proper visibility
    boost::call_once( init_tls_key, once_init_tls_key );

    // Get the exit handlers list for the current thread from tls.

    if( tls_key == invalid_tls_key )
    {
        return;
    }

    thread_exit_handlers* exit_handlers =
        static_cast< thread_exit_handlers* >( TlsGetValue( tls_key ) );

    // If a handlers list was found, invoke its handlers.

    if( exit_handlers != 0 )
    {
        // Call each handler and remove it from the list

        while( !exit_handlers->empty() )
        {
            if( thread_exit_handler exit_handler = *exit_handlers->begin() )
            {
                (*exit_handler)();
            }

            exit_handlers->pop_front();
        }

        // If TlsSetValue fails, we can't delete the list,
        // since a second call to on_thread_exit will try
        // to access it.

        if( TlsSetValue( tls_key, 0 ) )
        {
            delete exit_handlers;
        }
    }
}

#endif //defined(BOOST_HAS_WINTHREADS)
