// Copyright (C) 2001-2003
// William E. Kempf
// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#ifdef BOOST_HAS_MPTASKS

#include <boost/detail/workaround.hpp>

#include <boost/thread/once.hpp>
#include <cstdio>
#include <cassert>

#include <Multiprocessing.h>

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

namespace boost {

    void call_once(once_flag& flag, void (*func)())
{
    if(flag == false)
    {
        // all we do here is make a remote call to blue, as blue is not
        // reentrant.
        std::pair<void (*)(), once_flag *> sData(func, &flag);
        MPRemoteCall(remote_call_proxy, &sData, kMPOwningProcessRemoteContext);
        assert(flag == true);
    }
}

}
#endif
