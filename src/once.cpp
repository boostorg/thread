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

#ifndef BOOST_HAS_WINTHREADS

#include <boost/detail/workaround.hpp>

#include <boost/thread/once.hpp>
#include <cstdio>
#include <cassert>


#if defined(BOOST_HAS_MPTASKS)
#   include <Multiprocessing.h>
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
        once_callback* cb = reinterpret_cast<once_callback*>(
            pthread_getspecific(key));
        (**cb)();
    }

}
#elif defined(BOOST_HAS_MPTASKS)
namespace {
void *remote_call_proxy(void *pData)
{
    std::pair<void (*)(), boost::once_flag *> &rData(
        *reinterpret_cast<std::pair<void (*)(), boost::once_flag *> *>(pData));

    if(*rData.second == false)
    {
        rData.first();
        *rData.second = true;
    }

    return(NULL);
}
}

#endif

namespace boost {

void call_once(void (*func)(), once_flag& flag)
{
#if defined(BOOST_HAS_PTHREADS)
    pthread_once(&once, &key_init);
    pthread_setspecific(key, &func);
    pthread_once(&flag, do_once);
#elif defined(BOOST_HAS_MPTASKS)
    if(flag == false)
    {
        // all we do here is make a remote call to blue, as blue is not
        // reentrant.
        std::pair<void (*)(), once_flag *> sData(func, &flag);
        MPRemoteCall(remote_call_proxy, &sData, kMPOwningProcessRemoteContext);
        assert(flag == true);
    }
#endif
}

}

#endif

// Change Log:
//   1 Aug 01  WEKEMPF Initial version.
//   6 Sep 05  Anthony Williams. Removed win32 implementation
