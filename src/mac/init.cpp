// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include "init.hpp"

#include "remote_call_manager.hpp"


#include <boost/thread/detail/singleton.hpp>

#include <Multiprocessing.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


namespace {

// force these to get called by the end of static initialization time.
static bool g_bInitialized = (thread_init() && create_singletons());

}


bool thread_init()
{
    static bool bResult = MPLibraryIsLoaded();

    return(bResult);
}

bool create_singletons()
{
    using ::boost::detail::thread::singleton;

    singleton<remote_call_manager>::instance();

    return(true);
}


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost
