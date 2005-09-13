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
# define BOOST_WAIT_FOR_SINGLE_OBJECT ::WaitForSingleObject
# define BOOST_INFINITE INFINITE
#elif defined( WIN32 ) || defined( _WIN32 ) || defined( __WIN32__ )
namespace boost
{
    namespace detail
    {
        extern "C" __declspec(dllimport) int __stdcall CloseHandle(void*);
        extern "C" __declspec(dllimport) int __stdcall ReleaseMutex(void*);
        extern "C" struct _SECURITY_ATTRIBUTES;
        extern "C" __declspec(dllimport) void* __stdcall CreateMutexA(_SECURITY_ATTRIBUTES*,int,char const*);
        extern "C" __declspec(dllimport) unsigned long __stdcall GetCurrentProcessId();
        extern "C" __declspec(dllimport) unsigned long __stdcall WaitForSingleObject(void*,unsigned long);
    }
}
# define BOOST_CLOSE_HANDLE ::boost::detail::CloseHandle
# define BOOST_RELEASE_MUTEX ::boost::detail::ReleaseMutex
# define BOOST_CREATE_MUTEX ::boost::detail::CreateMutexA
# define BOOST_GET_PROCESS_ID ::boost::detail::GetCurrentProcessId
# define BOOST_WAIT_FOR_SINGLE_OBJECT ::boost::detail::WaitForSingleObject
# define BOOST_INFINITE 0xffffffff
#else
# error "Win32 functions not available"
#endif


#endif
