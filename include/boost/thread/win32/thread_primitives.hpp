#ifndef BOOST_WIN32_THREAD_PRIMITIVES_HPP
#define BOOST_WIN32_THREAD_PRIMITIVES_HPP

//  win32_thread_primitives.hpp
//
//  (C) Copyright 2005-6 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined( BOOST_USE_WINDOWS_H )
# include <windows.h>
namespace boost
{
    namespace detail
    {
        namespace win32
        {
            typedef ULONG_PTR ulong_ptr;
            typedef HANDLE handle;
            unsigned const infinite=INFINITE;
            unsigned const timeout=WAIT_TIMEOUT;

            using ::CreateMutexA;
            using ::CreateEventA;
            using ::CreateSemaphoreA;
            using ::CloseHandle;
            using ::ReleaseMutex;
            using ::ReleaseSemaphore;
            using ::SetEvent;
            using ::ResetEvent;
            using ::WaitForMultipleObjects;
            using ::WaitForSingleObject;
            using ::GetCurrentProcessId;
            using ::GetCurrentThreadId;
            using ::GetCurrentThread;
            using ::GetCurrentProcess;
            using ::DuplicateHandle;
            using ::SleepEx;
            using ::QueueUserAPC;
        }
    }
}
#elif defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ )
namespace boost
{
    namespace detail
    {
        namespace win32
        {
            
# ifdef _WIN64
            typedef unsigned __int64 ulong_ptr;
# else
            typedef unsigned long ulong_ptr;
# endif
            typedef void* handle;
            unsigned const infinite=~0U;
            unsigned const timeout=258U;

            extern "C"
            {
                struct _SECURITY_ATTRIBUTES;
                __declspec(dllimport) void* __stdcall CreateMutexA(_SECURITY_ATTRIBUTES*,int,char const*);
                __declspec(dllimport) void* __stdcall CreateSemaphoreA(_SECURITY_ATTRIBUTES*,long,long,char const*);
                __declspec(dllimport) void* __stdcall CreateEventA(_SECURITY_ATTRIBUTES*,int,int,char const*);
                __declspec(dllimport) int __stdcall CloseHandle(void*);
                __declspec(dllimport) int __stdcall ReleaseMutex(void*);
                __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
                __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
                __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void*,unsigned long);
                __declspec(dllimport) int __stdcall ReleaseSemaphore(void*,long,long*);
                __declspec(dllimport) void* __stdcall GetCurrentThread();
                __declspec(dllimport) void* __stdcall GetCurrentProcess();
                __declspec(dllimport) int __stdcall DuplicateHandle(void*,void*,void*,void**,unsigned long,int,unsigned long);
                __declspec(dllimport) unsigned long __stdcall SleepEx(unsigned long,int);
                typedef void (__stdcall *queue_user_apc_callback_function)(ulong_ptr);
                __declspec(dllimport) unsigned long __stdcall QueueUserAPC(queue_user_apc_callback_function,void*,ulong_ptr);
                __declspec(dllimport) int __stdcall SetEvent(void*);
                __declspec(dllimport) int __stdcall ResetEvent(void*);
                __declspec(dllimport) unsigned long __stdcall WaitForMultipleObjects(unsigned long nCount,void* const * lpHandles,int bWaitAll,unsigned long dwMilliseconds);
            }
        }
    }
}
#else
# error "Win32 functions not available"
#endif

namespace boost
{
    namespace detail
    {
        namespace win32
        {
            enum event_type
            {
                auto_reset_event=false,
                manual_reset_event=true
            };
            
            enum initial_event_state
            {
                event_initially_reset=false,
                event_initially_set=true
            };
            
            inline handle create_anonymous_event(event_type type,initial_event_state state)
            {
                return CreateEventA(0,type,state,0);
            }

            inline handle create_anonymous_semaphore(long initial_count,long max_count)
            {
                return CreateSemaphoreA(NULL,initial_count,max_count,NULL);
            }
        }
    }
}


#endif
