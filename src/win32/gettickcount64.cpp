// (C) Copyright 2014 Vicente J. Botet Escriba
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/win32/gettickcount64.hpp>

#include <boost/thread/once.hpp>

#ifdef BOOST_USE_WINDOWS_H
#include <Windows.h>
#else
extern "C"
{
  __declspec(dllimport)  boost::detail::win32::gettickcount32fn __stdcall GetProcAddress(void *, const char *);
  __declspec(dllimport) void * __stdcall GetModuleHandleW(const wchar_t *);
}
#endif

namespace boost
{
  namespace detail
  {
    namespace win32
    {
      namespace
      {
        gettickcount64fn gettickcount64 = NULL;
        gettickcount32fn gettickcount32 = NULL;
        ::boost::once_flag initfnonce = BOOST_ONCE_INIT;

        void init_gettickcount64()
        {
          gettickcount64 = reinterpret_cast<gettickcount64fn> (::GetProcAddress(::GetModuleHandleW(L"Kernel32.dll"),
              "GetTickCount64"));
          if (!gettickcount64) gettickcount32 = reinterpret_cast<gettickcount32fn> (::GetProcAddress(
              ::GetModuleHandleW(L"Kernel32.dll"), "GetTickCount"));
        }
      }

      ticks_type GetTickCount64()
      {
        ::boost::call_once(&init_gettickcount64, initfnonce);
        return gettickcount64 ? gettickcount64() : static_cast<ticks_type> (gettickcount32());
      }
    }
  }
}
