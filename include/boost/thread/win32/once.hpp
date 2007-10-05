#ifndef BOOST_THREAD_WIN32_ONCE_HPP
#define BOOST_THREAD_WIN32_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2005-7 Anthony Williams 
//  (C) Copyright 2005 John Maddock
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <cstring>
#include <cstddef>
#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
    using ::strlen;
    using ::memcpy;
    using ::ptrdiff_t;
}
#endif

namespace boost
{
    typedef long once_flag;

#define BOOST_ONCE_INIT 0

    namespace detail
    {
        struct win32_mutex_scoped_lock
        {
            void* const mutex_handle;
            explicit win32_mutex_scoped_lock(void* mutex_handle_):
                mutex_handle(mutex_handle_)
            {
                unsigned long const res=win32::WaitForSingleObject(mutex_handle,win32::infinite);
                BOOST_ASSERT(!res);
            }
            ~win32_mutex_scoped_lock()
            {
                bool const success=win32::ReleaseMutex(mutex_handle)!=0;
                BOOST_ASSERT(success);
            }
        };

        template <class I>
        void int_to_string(I p, char* buf)
        {
            unsigned i=0;
            for(; i < sizeof(I)*2; ++i)
            {
                buf[i] = 'A' + static_cast<char>((p >> (i*4)) & 0x0f);
            }
            buf[i] = 0;
        }

        unsigned const once_mutex_name_fixed_length=48;
        unsigned const once_mutex_name_fixed_buffer_size=once_mutex_name_fixed_length+1;
        unsigned const once_mutex_name_length=once_mutex_name_fixed_buffer_size+sizeof(void*)*2+sizeof(unsigned long)*2;
        
        // create a named mutex. It doesn't really matter what this name is
        // as long as it is unique both to this process, and to the address of "flag":
        inline void* create_once_mutex(char (&mutex_name)[once_mutex_name_length],void* flag_address)
        {
            static const char fixed_mutex_name[]="{C15730E2-145C-4c5e-B005-3BC753F42475}-once-flag";
            BOOST_STATIC_ASSERT(sizeof(fixed_mutex_name) == once_mutex_name_fixed_buffer_size);

            std::memcpy(mutex_name,fixed_mutex_name,sizeof(fixed_mutex_name));
            BOOST_ASSERT(sizeof(mutex_name) == std::strlen(mutex_name) + sizeof(void*)*2 + sizeof(unsigned long)*2 + 1);

            BOOST_STATIC_ASSERT(sizeof(void*) == sizeof(std::ptrdiff_t));
            detail::int_to_string(reinterpret_cast<std::ptrdiff_t>(flag_address), mutex_name + once_mutex_name_fixed_length);
            detail::int_to_string(win32::GetCurrentProcessId(), mutex_name + once_mutex_name_fixed_length + sizeof(void*)*2);
            BOOST_ASSERT(sizeof(mutex_name) == std::strlen(mutex_name) + 1);

            return win32::CreateMutexA(NULL, 0, mutex_name);
        }
        
    }
    

    template<typename Function>
    void call_once(once_flag& flag,Function f)
    {
        // Try for a quick win: if the proceedure has already been called
        // just skip through:
        long const function_complete_flag_value=0xc15730e2;

        if(::boost::detail::interlocked_read_acquire(&flag)!=function_complete_flag_value)
        {
            char mutex_name[::boost::detail::once_mutex_name_length];
            void* const mutex_handle(::boost::detail::create_once_mutex(mutex_name,&flag));
            BOOST_ASSERT(mutex_handle);
            detail::win32::handle_manager const closer(mutex_handle);
            detail::win32_mutex_scoped_lock const lock(mutex_handle);
      
            if(flag!=function_complete_flag_value)
            {
                f();
                BOOST_INTERLOCKED_EXCHANGE(&flag,function_complete_flag_value);
            }
        }
    }
}

#endif
