// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.  
// It is provided "as is" without express or implied warranty.

#include <boost/thread/once.hpp>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

namespace boost {

void call_once(void (*func)(), once_flag& flag)
{
#if defined(BOOST_HAS_WINTHREADS)
	if (!flag)
	{
        wchar_t name[37];
        swprintf(name, L"2AC1A572DB6944B0A65C38C4140AF2F4%X%X", GetCurrentProcessId(), &flag);
		HANDLE mutex = CreateMutexW(NULL, FALSE, name);
        assert(mutex != NULL);
		int res = WaitForSingleObject(mutex, INFINITE);
        assert(res == WAIT_OBJECT_0);
		if (!flag)
		{
			func();
			flag = true;
		}
		res = ReleaseMutex(mutex);
        assert(res);
		res = CloseHandle(mutex);
        assert(res);
	}
#elif defined(BOOST_HAS_PTHREADS)
	pthread_once(&flag, func);
#endif
}

}