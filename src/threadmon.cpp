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

#include <boost/thread/detail/config.hpp>

#if defined(BOOST_HAS_WINTHREADS)
    extern "C" void tss_cleanup_implemented(void);
    
    void require_tss_cleanup_implemented(void)
    {
        tss_cleanup_implemented();
    }

    #define WIN32_LEAN_AND_MEAN  
        //Exclude rarely-used stuff from Windows headers

    #include <windows.h>

    #ifdef BOOST_MSVC
    #   pragma warning(disable : 4786)
    #endif

    #include <list>
    #include <boost/thread/detail/threadmon.hpp>

    typedef void (__cdecl * handler)(void);
    typedef std::list<handler> exit_handlers;

    namespace
    {
        const DWORD invalid_key = TLS_OUT_OF_INDEXES;
        DWORD key = invalid_key;

        bool on_thread_enter_being_called = false;
        LONG attached_thread_count = 0;

        //See comments on InterlockedCompareExchange() in once.cpp
        //for an explanation of the following:

        inline LONG interlocked_inc_dec_wrapper(LONG (__stdcall *ice)(LONG*),
            volatile LONG* arg)
        {
            return (*ice)(const_cast<LONG*>(arg));
        }

        inline LONG interlocked_inc_dec_wrapper(LONG (__stdcall *ice)(volatile LONG*),
            volatile LONG* arg)
        {
            return (*ice)(arg);
        }

        inline LONG interlocked_inc_dec_wrapper(LPVOID (__stdcall *ice)(LPVOID*),
            volatile LONG* arg)
        {
            return (LONG)(*ice)((LPVOID*)arg);
        }

        // The friendly form of InterlockedIncrement that defers
        // according to the above function type wrappers.
        inline LONG interlocked_increment(volatile LPLONG arg)
        {
            return interlocked_inc_dec_wrapper(&InterlockedIncrement, arg);
        }

        // The friendly form of InterlockedDecrement that defers
        // according to the above function type wrappers.
        inline LONG interlocked_decrement(volatile LPLONG arg)
        {
            return interlocked_inc_dec_wrapper(&InterlockedDecrement, arg);
        }
    }

    extern "C" 
    BOOST_THREAD_DECL int at_thread_exit(void (__cdecl * func)(void))
    {
        //Get the exit handlers for the current thread,
        //creating and registering one if it doesn't exist.

        if (key == invalid_key)
            key = TlsAlloc();

        if (key == invalid_key)
            return -1;

        exit_handlers* handlers = 
            static_cast<exit_handlers*>(TlsGetValue(key));

        if (!handlers)
        {
            try
            {
                handlers = new exit_handlers;
                
                //Handle broken implementations 
                //of operator new that don't throw
                if (!handlers)
                    return -1;
            }
            catch (...)
            {
                return -1;
            }

            //Attempt to set a TLS value for the new handlers.
            if (TlsSetValue(key, handlers))
            {
                interlocked_increment(&attached_thread_count);
            }
            else
            {
                delete handlers;
                return -1;
            }
        }

        //Attempt to add the handler to the list of exit handlers.
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

    extern "C" BOOST_THREAD_DECL void on_process_enter(void)
    {
        if (key == invalid_key)
            key = TlsAlloc();

        if (key != invalid_key)
        {
            //Since the on_process_*() functions are working,
            //don't let TlsFree() be called until on_process_exit().
            interlocked_increment(&attached_thread_count);
        }
    }

    extern "C" BOOST_THREAD_DECL void on_process_exit(void)
    {
        if (key != invalid_key)
        {
            if (interlocked_decrement(&attached_thread_count) == 0)
            {
                TlsFree(key);
                key = invalid_key;
            }
        }
    }

    extern "C" BOOST_THREAD_DECL void on_thread_enter(void)
    {}

    extern "C" BOOST_THREAD_DECL void on_thread_exit(void)
    {
        if (key == invalid_key)
            return;

        exit_handlers* handlers = 
            static_cast<exit_handlers*>(TlsGetValue(key));

        if (handlers)
        {
            for (exit_handlers::iterator it = handlers->begin();
                it != handlers->end(); 
                ++it)
            {
                //Call each exit handler
                (*it)();
            }

            //Destroy the exit handlers
            if (TlsSetValue(key, 0))
            {
                if (interlocked_decrement(&attached_thread_count) == 0)
                {
                    TlsFree(key);
                    key = invalid_key;
                }

                delete handlers;
            }
        }
    }

    #if defined(BOOST_THREAD_BUILD_DLL)
        extern "C" void tss_cleanup_implemented(void)
        {
            //Don't need to do anything; this function's 
            //sole purpose is to cause a link error in cases
            //where tss cleanup is not implemented by Boost.Threads
            //as a reminder that user code is responsible for calling
            //on_process_enter(), on_thread_exit(), and
            //on_process_exit() at the appropriate times
            //and implementing an empty tss_cleanup_implemented()
            //function to eliminate the link error.
        }

        #if defined(__BORLANDC__)
        #define DllMain DllEntryPoint
        #endif

        extern "C"
        BOOL WINAPI DllMain(HANDLE /*module*/, DWORD reason, LPVOID)
        {
            switch (reason)
            {
                case DLL_PROCESS_ATTACH:
                {
                    on_process_enter();
                    on_thread_enter();
                    break;
                }
                case DLL_THREAD_ATTACH:
                {
                    on_thread_enter();
                    break;
                }
                case DLL_THREAD_DETACH:
                {
                    on_thread_exit();
                    break;
                }
                case DLL_PROCESS_DETACH:
                {
                    on_thread_exit();
                    on_process_exit();
                    break;
                }
            }
            return TRUE;
        }
    #elif defined(BOOST_THREAD_BUILD_LIB)
        #if defined(BOOST_MSVC) && (BOOST_MSVC >= 1310) //1310 == VC++ 7.1
            //As currently defined, the following is known
            //to work only for VC++ 7.1.
            //It is known not to work with VC 6.
            #include <libs/thread/src/pe_tls.ipp>
        #endif
    #endif // BOOST_THREAD_BUILD_DLL
#endif // BOOST_HAS_WINTHREADS

// Change Log:
//  20 Mar 04  GLASSFORM for WEKEMPF 
//      Removed uneccessary critical section:
//          Windows already serializes calls to DllMain.
//      Removed registered_handlers.
