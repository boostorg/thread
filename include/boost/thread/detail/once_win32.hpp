#ifndef BOOST_WIN32_ONCE_HPP
#define BOOST_WIN32_ONCE_HPP

//  once.hpp
//
//  (C) Copyright 2005 Anthony Williams 
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
#include <boost/thread/detail/win32_thread_primitives.hpp>

#ifdef BOOST_NO_STDC_NAMESPACE
namespace std
{
    using ::strlen;
}
#endif

namespace boost
{
    typedef long once_flag;

#define BOOST_ONCE_INIT 0

    namespace detail
    {
        struct handle_closer
        {
            void* const handle_to_close;
            handle_closer(void* handle_to_close_):
                handle_to_close(handle_to_close_)
            {}
            ~handle_closer()
            {
                BOOST_CLOSE_HANDLE(handle_to_close);
            }
        };

        struct mutex_releaser
        {
            void* const mutex_to_release;
            mutex_releaser(void* mutex_to_release_):
                mutex_to_release(mutex_to_release_)
            {}
            ~mutex_releaser()
            {
                BOOST_RELEASE_MUTEX(mutex_to_release);
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
    }
    

    template<typename Function>
    void call_once(Function f,once_flag& flag)
    {
        //
        // Try for a quick win: if the proceedure has already been called
        // just skip through:
        //
        long const function_complete_flag_value=0xc15730e2;

        if(!BOOST_INTERLOCKED_COMPARE_EXCHANGE(&flag,function_complete_flag_value,
                                               function_complete_flag_value))
        {
            //
            // create a name for our mutex, it doesn't really matter what this name is
            // as long as it is unique both to this process, and to the address of "flag":
            //
            char mutex_name[49+sizeof(void*)*2+sizeof(unsigned long)*2] = { "{C15730E2-145C-4c5e-B005-3BC753F42475}-once-flag" };
            BOOST_ASSERT(sizeof(mutex_name) == std::strlen(mutex_name) + sizeof(void*)*2 + sizeof(unsigned long)*2 + 1);
            BOOST_STATIC_ASSERT(sizeof(void*) == sizeof(std::ptrdiff_t));
            detail::int_to_string(reinterpret_cast<std::ptrdiff_t>(&flag), mutex_name + 48);
            detail::int_to_string(BOOST_GET_PROCESS_ID(), mutex_name + 48 + sizeof(void*)*2);
            BOOST_ASSERT(sizeof(mutex_name) == std::strlen(mutex_name) + 1);

            void* const mutex_handle(BOOST_CREATE_MUTEX(NULL, 0, mutex_name));
            BOOST_ASSERT(mutex_handle);
            detail::handle_closer const closer(mutex_handle);
            BOOST_WAIT_FOR_SINGLE_OBJECT(mutex_handle,BOOST_INFINITE);
            detail::mutex_releaser const releaser(mutex_handle);
      
            if(!BOOST_INTERLOCKED_COMPARE_EXCHANGE(&flag,function_complete_flag_value,
                                                   function_complete_flag_value))
            {
                f();
                BOOST_INTERLOCKED_EXCHANGE(&flag,function_complete_flag_value);
            }
        }
    }
}

#endif
