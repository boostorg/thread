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
#include <cstdio>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
    namespace std { using ::sprintf; }
#endif

#if defined(BOOST_HAS_PTHREADS)
namespace {
pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;

typedef void (*once_callback)();
}

extern "C" {

static void key_init()
{
    pthread_key_create(&key, 0);
}

static void do_once()
{
    once_callback cb = reinterpret_cast<once_callback>(pthread_getspecific(key));
    (*cb)();
}

}
#endif

namespace boost {

void call_once(void (*func)(), once_flag& flag)
{
#if defined(BOOST_HAS_WINTHREADS)
    once_flag tmp = flag;

    // Memory barrier would be needed here to prevent race conditions on some platforms with
    // partial ordering.

	if (!tmp)
	{
        char name[41];
        std::sprintf(name, "2AC1A572DB6944B0A65C38C4140AF2F4%X%X", GetCurrentProcessId(), &flag);
		HANDLE mutex = CreateMutex(NULL, FALSE, name);
        assert(mutex != NULL);

        int res = 0;
        res = WaitForSingleObject(mutex, INFINITE);
        assert(res == WAIT_OBJECT_0);

        tmp = flag;
		if (!tmp)
		{
			func();
            tmp = true;

            // Memory barrier would be needed here to prevent race conditions on some platforms
            // with partial ordering.

			flag = tmp;
		}

		res = ReleaseMutex(mutex);
        assert(res);
        res = CloseHandle(mutex);
        assert(res);
	}
#elif defined(BOOST_HAS_PTHREADS)
    pthread_once(&once, &key_init);
    pthread_setspecific(key, func);
	pthread_once(&flag, do_once);
#endif
}

}

// Change Log:
//   1 Aug 01  WEKEMPF Initial version.
