// threadmon.cpp : Defines the entry point for the DLL application.
//

#define BOOST_THREADMON_EXPORTS
#include "threadmon.hpp"

#ifdef BOOST_HAS_WINTHREADS

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#ifdef BOOST_MSVC
#   pragma warning(disable : 4786)
#endif

#include <list>
#include <set>
#include <algorithm>

typedef void (__cdecl * handler)(void);
typedef std::list<handler> exit_handlers;

namespace
{
    DWORD key;
}

#if defined(__BORLANDC__)
#define DllMain DllEntryPoint
#endif

extern "C"
BOOL WINAPI DllMain(HANDLE module, DWORD reason, LPVOID)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            key = TlsAlloc();
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            {
                // Call the thread's exit handlers.
                exit_handlers* handlers = static_cast<exit_handlers*>(TlsGetValue(key));
                if (handlers)
                {
                    for (exit_handlers::iterator it = handlers->begin(); it != handlers->end(); ++it)
                        (*it)();

                    delete handlers;
                }
            }
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

int on_thread_exit(void (__cdecl * func)(void))
{
    // Get the exit handlers for the current thread, creating and registering
    // one if it doesn't exist.
    exit_handlers* handlers = static_cast<exit_handlers*>(TlsGetValue(key));
    if (!handlers)
    {
        try
        {
            handlers = new exit_handlers;
            // Handle "broken" implementations of operator new that don't throw.
            if (!handlers)
                return -1;
        }
        catch (...)
        {
            return -1;
        }

        // Attempt to set a TLS value for the new handlers.
        if (!TlsSetValue(key, handlers))
        {
            delete handlers;
            return -1;
        }
    }

    // Attempt to add the handler to the list of exit handlers.
    try
    {
        handlers->push_front(func);
    }
    catch (...)
    {
        return -1;
    }

    return 0;
}

#endif // BOOST_HAS_WINTHREADS
