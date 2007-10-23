// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
#ifndef THREAD_HEAP_ALLOC_HPP
#define THREAD_HEAP_ALLOC_HPP
#include <new>
#include "thread_primitives.hpp"

#if defined( BOOST_USE_WINDOWS_H )
# include <windows.h>

namespace boost
{
    namespace detail
    {
        namespace win32
        {
            using ::GetProcessHeap;
            using ::HeapAlloc;
            using ::HeapFree;
        }
    }
}

#else

namespace boost
{
    namespace detail
    {
        namespace win32
        {
            extern "C"
            {
                __declspec(dllimport) handle __stdcall GetProcessHeap();
                __declspec(dllimport) void* __stdcall HeapAlloc(handle,unsigned long,ulong_ptr);
                __declspec(dllimport) int __stdcall HeapFree(handle,unsigned long,void*);
            }
        }
    }
}

#endif

namespace boost
{
    namespace detail
    {
        template<typename T>
        T* heap_new()
        {
            void* const heap_memory=detail::win32::HeapAlloc(detail::win32::GetProcessHeap(),0,sizeof(T));
            T* const data=new (heap_memory) T();
            return data;
        }

        template<typename T,typename A1>
        T* heap_new(A1 a1)
        {
            void* const heap_memory=detail::win32::HeapAlloc(detail::win32::GetProcessHeap(),0,sizeof(T));
            T* const data=new (heap_memory) T(a1);
            return data;
        }
        
        template<typename T,typename A1,typename A2>
        T* heap_new(A1 a1,A2 a2)
        {
            void* const heap_memory=detail::win32::HeapAlloc(detail::win32::GetProcessHeap(),0,sizeof(T));
            T* const data=new (heap_memory) T(a1,a2);
            return data;
        }
        
        template<typename T>
        void heap_delete(T* data)
        {
            data->~T();
            detail::win32::HeapFree(detail::win32::GetProcessHeap(),0,data);
        }

        template<typename T>
        struct do_delete
        {
            T* data;
            
            do_delete(T* data_):
                data(data_)
            {}
            
            void operator()() const
            {
                detail::heap_delete(data);
            }
        };

        template<typename T>
        do_delete<T> make_heap_deleter(T* data)
        {
            return do_delete<T>(data);
        }
    }
}


#endif
