#ifndef BOOST_WIN32_THREAD_PRIMITIVES_HPP
#define BOOST_WIN32_THREAD_PRIMITIVES_HPP

//  win32_thread_primitives.hpp
//
//  (C) Copyright 2005 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

#if defined( BOOST_USE_WINDOWS_H )
# include <windows.h>
# define BOOST_CLOSE_HANDLE ::CloseHandle
# define BOOST_RELEASE_MUTEX ::ReleaseMutex
# define BOOST_CREATE_MUTEX ::CreateMutexA
# define BOOST_GET_PROCESS_ID ::GetCurrentProcessId
# define BOOST_GET_CURRENT_THREAD_ID ::GetCurrentThreadId
# define BOOST_WAIT_FOR_SINGLE_OBJECT ::WaitForSingleObject
# define BOOST_CREATE_SEMAPHORE ::CreateSemaphoreA
# define BOOST_RELEASE_SEMAPHORE ::ReleaseSemaphore
# define BOOST_GET_CURRENT_THREAD ::GetCurrentThread
# define BOOST_GET_CURRENT_PROCESS ::GetCurrentProcess
# define BOOST_DUPLICATE_HANDLE ::DuplicateHandle
# define BOOST_SLEEP_EX ::SleepEx
# define BOOST_QUEUE_USER_APC ::QueueUserAPC
# define BOOST_INFINITE INFINITE
namespace boost
{
    namespace detail
    {
        typedef ULONG_PTR ulong_ptr;

        extern "C"
        {
            inline BOOL CloseHandle(HANDLE h)
            {
                return ::CloseHandle(h);
            }
            inline int ReleaseMutex(HANDLE h)
            {
                return ::ReleaseMutex(h);
            }
            inline HANDLE CreateMutexA(::_SECURITY_ATTRIBUTES* sa,BOOL owner,char const* name)
            {
                return ::CreateMutexA(sa,owner,name);
            }
            inline HANDLE CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes,BOOL bManualReset,
                                                 BOOL bInitialState,LPCSTR lpName)
            {
                return ::CreateEventA(lpEventAttributes,bManualReset,bInitialState,lpName);
            }

            inline BOOL ReleaseSemaphore(HANDLE hSemaphore,LONG lReleaseCount,LPLONG lpPreviousCount)
            {
                return ::ReleaseSemaphore(hSemaphore,lReleaseCount,lpPreviousCount);
            }

            inline HANDLE CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,LONG lInitialCount,
                                                     LONG lMaximumCount,LPCSTR lpName)
            {
                return ::CreateSemaphoreA(lpSemaphoreAttributes,lInitialCount,lMaximumCount,lpName);
            }
            inline BOOL SetEvent(HANDLE hEvent)
            {
                return ::SetEvent(hEvent);
            }
            inline BOOL ResetEvent(HANDLE hEvent)
            {
                return ::ResetEvent(hEvent);
            }


//             inline unsigned long GetCurrentProcessId();
//             inline unsigned long GetCurrentThreadId();
//             inline unsigned long WaitForSingleObject(HANDLE,unsigned long);
//             inline HANDLE GetCurrentThread();
//             inline HANDLE GetCurrentProcess();
//             inline int DuplicateHandle(HANDLE,HANDLE,HANDLE,HANDLE*,unsigned long,int,unsigned long);
//             inline unsigned long SleepEx(unsigned long,int);
//             typedef void (*queue_user_apc_callback_function)(ulong_ptr);
//             inline unsigned long QueueUserAPC(queue_user_apc_callback_function,HANDLE,ulong_ptr);
        }
    }
}
#elif defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ )
namespace boost
{
    namespace detail
    {
# ifdef _WIN64
        typedef unsigned __int64 ulong_ptr;
# else
        typedef unsigned long ulong_ptr;
# endif

        extern "C"
        {
            __declspec(dllimport) int __stdcall CloseHandle(void*);
            __declspec(dllimport) int __stdcall ReleaseMutex(void*);
            struct _SECURITY_ATTRIBUTES;
            __declspec(dllimport) void* __stdcall CreateMutexA(_SECURITY_ATTRIBUTES*,int,char const*);
            __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
            __declspec(dllimport) unsigned long __stdcall GetCurrentThreadId();
            __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void*,unsigned long);
            __declspec(dllimport) int __stdcall ReleaseSemaphore(void*,long,long*);
            __declspec(dllimport) void* __stdcall CreateSemaphoreA(_SECURITY_ATTRIBUTES*,long,long,char const*);
            __declspec(dllimport) void* __stdcall GetCurrentThread();
            __declspec(dllimport) void* __stdcall GetCurrentProcess();
            __declspec(dllimport) int __stdcall DuplicateHandle(void*,void*,void*,void**,unsigned long,int,unsigned long);
            __declspec(dllimport) unsigned long __stdcall SleepEx(unsigned long,int);
            typedef void (__stdcall *queue_user_apc_callback_function)(ulong_ptr);
            __declspec(dllimport) unsigned long __stdcall QueueUserAPC(queue_user_apc_callback_function,void*,ulong_ptr);
            __declspec(dllimport) void* CreateEventA(_SECURITY_ATTRIBUTES*,int,int,char const*);
            __declspec(dllimport) int __stdcall SetEvent(void*);
            __declspec(dllimport) int __stdcall ResetEvent(void*);
        }
    }
}
# define BOOST_CLOSE_HANDLE ::boost::detail::CloseHandle
# define BOOST_RELEASE_MUTEX ::boost::detail::ReleaseMutex
# define BOOST_CREATE_MUTEX ::boost::detail::CreateMutexA
# define BOOST_GET_PROCESS_ID ::boost::detail::GetCurrentProcessId
# define BOOST_GET_CURRENT_THREAD_ID ::boost::detail::GetCurrentThreadId
# define BOOST_WAIT_FOR_SINGLE_OBJECT ::boost::detail::WaitForSingleObject
# define BOOST_CREATE_SEMAPHORE ::boost::detail::CreateSemaphoreA
# define BOOST_RELEASE_SEMAPHORE ::boost::detail::ReleaseSemaphore
# define BOOST_GET_CURRENT_THREAD ::boost::detail::GetCurrentThread
# define BOOST_GET_CURRENT_PROCESS ::boost::detail::GetCurrentProcess
# define BOOST_DUPLICATE_HANDLE ::boost::detail::DuplicateHandle
# define BOOST_SLEEP_EX ::boost::detail::SleepEx
# define BOOST_QUEUE_USER_APC ::boost::detail::QueueUserAPC
# define BOOST_INFINITE 0xffffffff
#else
# error "Win32 functions not available"
#endif


#endif
