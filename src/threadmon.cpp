// threadmon.cpp : Defines the entry point for the DLL application.
//

#define BOOST_THREADMON_EXPORTS
#include "threadmon.hpp"

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

#pragma warning(disable : 4786)
#include <list>
#include <algorithm>

typedef void (__cdecl * handler)(void);
typedef std::list<handler> exit_handlers;

namespace
{
    CRITICAL_SECTION cs;
    DWORD key;
    std::list<exit_handlers*> hndlrlst;
}

BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID)
{
    switch (reason)
	{
		case DLL_PROCESS_ATTACH:
            InitializeCriticalSection(&cs);
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

                    // Remove the exit handler list from the registered lists and then destroy it.
                    EnterCriticalSection(&cs);
                    std::list<exit_handlers*>::iterator found = std::find(hndlrlst.begin(), hndlrlst.end(), handlers);
                    hndlrlst.erase(found);
                    LeaveCriticalSection(&cs);
                    delete handlers;
                }
            }
            break;
		case DLL_PROCESS_DETACH:
            {
                // Assume the main thread is ending (call its handlers) and all other threads
                // have already ended.  If this DLL is loaded and unloaded dynamically at run time
                // this is a bad assumption, but this is the best we can do.
                exit_handlers* handlers = static_cast<exit_handlers*>(TlsGetValue(key));
                if (handlers)
                {
                    for (exit_handlers::iterator it = handlers->begin(); it != handlers->end(); ++it)
                        (*it)();
                }

                // Destroy any remaining exit handlers.  Above we assumed there'd only be the main
                // thread left, but to insure we don't get memory leaks we won't make that assumption
                // here.
                EnterCriticalSection(&cs);
                for (std::list<exit_handlers*>::iterator it = hndlrlst.begin(); it != hndlrlst.end(); ++it)
                    delete (*it);
                LeaveCriticalSection(&cs);
                DeleteCriticalSection(&cs);
                TlsFree(key);
            }
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

        // Attempt to register this new handler so that memory can be properly
        // cleaned up.
        try
        {
            EnterCriticalSection(&cs);
            hndlrlst.push_back(handlers);
            LeaveCriticalSection(&cs);
        }
        catch (...)
        {
            LeaveCriticalSection(&cs);
            delete handlers;
            return -1;
        }
    }

    // Attempt to add the handler to the list of exit handlers. If it's been previously
    // added just report success and exit.
    try
    {
        exit_handlers::iterator it = std::find(handlers->begin(), handlers->end(), func);
        if (it != handlers->end())
            return 0;
        handlers->push_back(func);
    }
    catch (...)
    {
        return -1;
    }

    return 0;
}
